#ifndef PARALLEL_PARETO_QUEUE_H_
#define PARALLEL_PARETO_QUEUE_H_

#include <iostream>
#include <vector>

#include "options.hpp"

#define COMPUTE_PARETO_MIN
#include "datastructures/BTree.hpp"

#include <algorithm>
#include <limits>
#include "utility/datastructure/graph/GraphMacros.h"

#include "tbb/enumerable_thread_specific.h"
#include "tbb/cache_aligned_allocator.h"
#include "tbb/scalable_allocator.h"
#include "tbb/task.h"
#include "tbb/concurrent_vector.h"


#ifndef PARETO_FIND_RECURSION_END_LEVEL
#define PARETO_FIND_RECURSION_END_LEVEL 3
#endif



template<typename type>
struct BTreeSetOrderer {
	bool operator() (const type& i, const type& j) const {
		if (i.first_weight == j.first_weight) {
			if (i.second_weight == j.second_weight) {
				return i.node < j.node;
			}
			return i.second_weight < j.second_weight;
		}
		return i.first_weight < j.first_weight;
	}
};


/**
 * Queue storing all temporary labels of all nodes.
 */
template<typename graph_slot, typename data_type, typename label_type>
class ParallelBTreeParetoQueue : public btree<data_type, label_type, BTreeSetOrderer<data_type>> {
	friend class FindParetMinTask;
private:
	typedef btree<data_type, label_type, BTreeSetOrderer<data_type>> base_type;

	typedef typename graph_slot::NodeID NodeID;
	typedef typename graph_slot::EdgeID EdgeID;
	typedef typename graph_slot::Edge Edge;

	typedef typename data_type::weight_type weight_type;
	const data_type min_label;

	typedef typename base_type::node node;
	typedef typename base_type::inner_node inner_node;
	typedef typename base_type::leaf_node leaf_node;
	typedef typename base_type::width_type width_type;

	const graph_slot& graph;

	tbb::task_list root_tasks;

public:
	typedef std::vector< Operation<data_type>, tbb::scalable_allocator<Operation<data_type>> > OpVec; 
	typedef std::vector< label_type, tbb::scalable_allocator<label_type> > CandLabelVec;
	typedef std::vector< CandLabelVec > CandLabelVecVec;
	typedef std::vector< NodeID, tbb::scalable_allocator<NodeID> > NodeVec;
	typedef std::vector< data_type, tbb::scalable_allocator<data_type> > MinimaVec;

	struct NodeVecAffinity { 
		NodeVec nodes;
		tbb::task::affinity_id  affinity;
	};

	typedef tbb::enumerable_thread_specific< OpVec,           tbb::cache_aligned_allocator<OpVec>,           tbb::ets_key_per_instance > TLSUpdates; 
	typedef tbb::enumerable_thread_specific< CandLabelVecVec, tbb::cache_aligned_allocator<CandLabelVecVec>, tbb::ets_key_per_instance > TLSCandidates; 
	typedef tbb::enumerable_thread_specific< NodeVecAffinity, tbb::cache_aligned_allocator<NodeVec>,         tbb::ets_key_per_instance > TLSAffected;
	typedef tbb::enumerable_thread_specific< MinimaVec,       tbb::cache_aligned_allocator<MinimaVec>,       tbb::ets_key_per_instance > TLSMinima; 

	TLSUpdates tls_local_updates;
	TLSCandidates tls_candidates;
	TLSAffected tls_affected_nodes;
	TLSMinima tls_minima;

	typedef unsigned short thread_count;
	const thread_count num_threads;

	std::vector<tbb::atomic<thread_count>> candidate_bufferlist_counter;
	CandLabelVec** candidate_bufferlist; // two dimensional array [node ID][thread id]


public:

	ParallelBTreeParetoQueue(const graph_slot& _graph, const thread_count _num_threads)
		: base_type(_graph.numberOfNodes()), min_label(NodeID(0), typename data_type::label_type(std::numeric_limits<weight_type>::min(),
			std::numeric_limits<weight_type>::max())), graph(_graph), num_threads(_num_threads), candidate_bufferlist_counter(_graph.numberOfNodes())
	{
		candidate_bufferlist = (CandLabelVec**) malloc(graph.numberOfNodes() * _num_threads * sizeof(CandLabelVec*));
		assert(num_threads > 0);
	}

	~ParallelBTreeParetoQueue() {
		free(candidate_bufferlist);
	}

	void init(const data_type& data) {
		const std::vector<Operation<data_type>> upds = {{Operation<data_type>::INSERT, data}};
		base_type::apply_updates(upds);
	}

	template<typename T>
	void applyUpdates(const T* updates, const size_t update_count) {
		base_type::apply_updates(updates, update_count);
	}

	bool empty() {
		return base_type::empty();
	}

	size_t size() {
		return base_type::size();
	}

	void printStatistics() {
		std::cout << base_type::name() << " (k=" << base_type::traits::leafparameter_k << ", b=" << base_type::traits::branchingparameter_b << "):" << std::endl;
		std::cout << "  inner slots size [" << base_type::innerslotmin << ", " << base_type::innerslotmax << "]" << std::endl;
		std::cout << "  leaf slots size [" << base_type::leafslotmin << ", " << base_type::leafslotmax << "]" << std::endl;
	}

	void findParetoMinima() {
		if (base_type::root->level < PARETO_FIND_RECURSION_END_LEVEL) {
			findParetoMinAndDistribute(base_type::root, min_label);
		} else {
			const inner_node* const inner = (inner_node*) base_type::root;
			const width_type slotuse = inner->slotuse;

			label_type min = min_label;
			for (width_type i = 0; i<slotuse; ++i) {
				 if (inner->slot[i].minimum.second_weight < min.second_weight ||
						(inner->slot[i].minimum.first_weight == min.first_weight && inner->slot[i].minimum.second_weight == min.second_weight)) {
					root_tasks.push_back(*new(tbb::task::allocate_root()) FindParetMinTask(inner->slot[i].childid, min, this));
					min = inner->slot[i].minimum;
				}
			}
			tbb::task::spawn_root_and_wait(root_tasks);
			root_tasks.clear();
		}
	}

    class FindParetMinTask : public tbb::task {
       	const node* const in_node;
       	const label_type prefix_minima;
        ParallelBTreeParetoQueue* const tree;

    public:
		
		inline FindParetMinTask(const node* const _in_node, const label_type _prefix_minima, ParallelBTreeParetoQueue* const _tree) 
			: in_node(_in_node), prefix_minima(_prefix_minima), tree(_tree)
		{ }

		tbb::task* execute() {
			if (in_node->level < PARETO_FIND_RECURSION_END_LEVEL) {
				tree->findParetoMinAndDistribute(in_node, prefix_minima);
				return NULL;
			} else {
				const inner_node* const inner = (inner_node*) in_node;
				const width_type slotuse = inner->slotuse;

				tbb::task_list tasks;
				width_type task_count = 0;
				auto& c = *new(allocate_continuation()) tbb::empty_task(); // our parent will wait for this one

				label_type min = prefix_minima;
				for (width_type i = 0; i<slotuse; ++i) {
				 if (inner->slot[i].minimum.second_weight < min.second_weight ||
						(inner->slot[i].minimum.first_weight == min.first_weight && inner->slot[i].minimum.second_weight == min.second_weight)) {
						tasks.push_back(*new(c.allocate_child()) FindParetMinTask(inner->slot[i].childid, min, tree));
						++task_count;
						min = inner->slot[i].minimum;
					}
				}
				c.set_ref_count(task_count);
				auto& task = tasks.pop_front();
				spawn(tasks);
				return &task;
			}
		}

		virtual void note_affinity(tbb::task::affinity_id id) {
			typename TLSAffected::reference locally_affected_nodes = tree->tls_affected_nodes.local();
			locally_affected_nodes.affinity = id;
		}
    };

	void findParetoMinAndDistribute(const node* const in_node, const label_type& prefix_minima) {
		typename TLSMinima::reference minima = tls_minima.local();
		// Scan the tree while it is likely to be still in cache
		base_type::find_pareto_minima(in_node, prefix_minima, minima);

		typename TLSUpdates::reference local_updates = tls_local_updates.local();
		typename TLSAffected::reference locally_affected_nodes = tls_affected_nodes.local();
		typename TLSCandidates::reference local_candidates = tls_candidates.local();
		local_candidates.resize(graph.numberOfNodes());

		for (const data_type& min : minima) {
			// Schedule minima for deletion
			local_updates.push_back(Operation<data_type>(Operation<data_type>::DELETE, min));

			// Derive candidates
			FORALL_EDGES(graph, min.node, eid) {
				const Edge& edge = graph.getEdge(eid);
				if (local_candidates[edge.target].empty()) {
					const thread_count position = candidate_bufferlist_counter[edge.target].fetch_and_increment();
					candidate_bufferlist[edge.target * num_threads + position] = &(local_candidates[edge.target]);
					if (position == 0) {
						// We were the first, so we are responsible!
						locally_affected_nodes.nodes.push_back(edge.target);
					}
					assert(position < num_threads);
				}
				local_candidates[edge.target].push_back(createNewLabel(min, edge));
			}
		}
		minima.clear();
	}

	static label_type createNewLabel(const label_type& current_label, const Edge& edge) {
		return label_type(current_label.first_weight + edge.first_weight, current_label.second_weight + edge.second_weight);
	}

};


#endif