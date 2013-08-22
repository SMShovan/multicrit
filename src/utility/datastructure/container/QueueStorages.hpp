/*
 * QueueStorages.hpp
 *
 *  Created on: 17.02.2012
 *      Author: kobitzsch
 */

#ifndef QUEUESTORAGES_HPP_
#define QUEUESTORAGES_HPP_

#include "../../exception.h"

#include <cstring>
#include <limits>
#include <map>
#include <iostream>
#include <vector>
#include <tr1/unordered_map>

#ifdef USE_GOOGLE_DATASTRUCTURES
#include <google/dense_hash_map>
#endif

namespace utility{
namespace datastructure{


//TODO distinguish between find and operator[] for check/insert
template< typename id_slot >
class ArrayStorage{
public:
	ArrayStorage( id_slot size_, bool store_data = false )
		: size( size_ ),
		  positions( new size_t[size_] )
	{
		(void) store_data;
		memset( positions, 0, sizeof(size_t) * size );
	}

	~ArrayStorage(){
		if( positions )
			delete[] positions, positions = NULL;
	}

	const size_t & operator[]( id_slot node ) const {
		GUARANTEE( (size_t) node < size, std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
		return positions[node];
	}

	size_t & operator[]( id_slot node ){
		GUARANTEE( (size_t) node < size, std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
		return positions[node];
	}

	void clear(){ /* do nothing */ }
protected:
	size_t size;
	size_t* positions;
};

template< typename id_slot, typename data_slot >
class DataArrayStorage{
public:
	DataArrayStorage( id_slot size_ )
		: size( size_ ),
		  data( new data_slot[size_] )
	{
		memset( data, 0, sizeof(data_slot) * size );
	}

	~DataArrayStorage(){
		if( data )
			delete[] data, data = NULL;
	}

	const data_slot & operator[]( id_slot node ) const {
		GUARANTEE( (size_t) node < size, std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
		return data[node];
	}

	data_slot & operator[]( id_slot node ){
		GUARANTEE( (size_t) node < size, std::runtime_error, "[error] ArrayStorage::accessing non-existing element." )
		return data[node];
	}

	void clear(){ /* do nothing */ }
protected:
	size_t size;
	data_slot* data;
};

template< typename id_slot >
class MapStorage{
public:
	MapStorage( id_slot size = 0 ){ /* do nothing */ }

	const size_t & operator[]( id_slot node ) const {
		return node_positions[node];
	}

	size_t & operator[]( id_slot node ){
		return node_positions[node];
	}

	void clear(){
		node_positions.clear();
	}
protected:
	std::map< id_slot, size_t > node_positions;
};

template< typename id_slot >
class UnorderedMapStorage{
public:
	UnorderedMapStorage( id_slot size = 0 ){ /* do nothing */ }

	const size_t & operator[]( id_slot node ) const {
		return node_positions[node];
	}

	size_t & operator[]( id_slot node ){
		return node_positions[node];
	}

	void clear(){
		node_positions.clear();
	}
protected:
	std::tr1::unordered_map< id_slot, size_t > node_positions;
};

#ifdef USE_GOOGLE_DATASTRUCTURES
template< typename id_slot >
class DenseHashStorage{
public:
	DenseHashStorage( id_slot empty_node = std::numeric_limits< id_slot >::max() ){
		node_positions.set_empty_key_slot( empty_node );
	}

	const size_t & operator[]( id_slot node ) const {
		return node_positions[node];
	}

	size_t & operator[]( id_slot node ){
		return node_positions[node];
	}

	void clear(){
		node_positions.clear();
	}
protected:
	google::dense_hash_map< id_slot, size_t > node_positions;
};
#endif

}
}


#endif /* QUEUESTORAGES_HPP_ */
