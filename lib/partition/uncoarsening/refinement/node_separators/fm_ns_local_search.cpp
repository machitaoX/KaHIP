//
// Author: Christian Schulz <christian.schulz@kit.edu>
// 

#include "fm_ns_local_search.h"
#include "data_structure/priority_queues/maxNodeHeap.h"
#include "tools/random_functions.h"
#include "tools/quality_metrics.h"

fm_ns_local_search::fm_ns_local_search() {
                
}

fm_ns_local_search::~fm_ns_local_search() {
                
}

EdgeWeight fm_ns_local_search::perform_refinement(const PartitionConfig & config, graph_access & G) {

        std::vector< maxNodeHeap > queues; queues.resize(2);
        std::vector< bool > moved_out_of_separator(G.number_of_nodes(), false);
        std::vector< change_set > rollback_info;

        forall_nodes(G, node) {
                if( G.getPartitionIndex(node) == 2 ) {
                        Gain toLHS = 0;
                        Gain toRHS = 0;
                        compute_gain( G, node, toLHS, toRHS);

                        queues[0].insert(node, toLHS);
                        queues[1].insert(node, toRHS);
                }
        } endfor
        
        std::vector< NodeWeight > block_weights(3,0);
        forall_nodes(G, node) {
                if( G.getPartitionIndex(node) == 0) {
                        block_weights[0] += G.getNodeWeight(node);
                } else if( G.getPartitionIndex(node) == 1 ) {
                        block_weights[1] += G.getNodeWeight(node);
                } else {
                        block_weights[2] += G.getNodeWeight(node);
                }
        } endfor

        int max_number_of_swaps = 10;
        NodeWeight best_separator = block_weights[2];
        int undo_idx = 0;

        //roll forwards
        Gain gainToA = queues[0].maxValue();
        Gain gainToB = queues[1].maxValue();

        while( gainToA > 0 || gainToB > 0) {
                Gain top_gain = gainToA > gainToB ? gainToA : gainToB;
                Gain other_gain = gainToA > gainToB ? gainToB : gainToA;

                PartitionID to_block    = top_gain == gainToA ? 0 : 1;
                PartitionID other_block = to_block == 0 ? 1 : 0;

                NodeID nodeToBlock = queues[to_block].maxElement();
                if( block_weights[to_block] + G.getNodeWeight(nodeToBlock) < config.upper_bound_partition ) {
                        queues[to_block].deleteMax();
                        queues[other_block].deleteNode(nodeToBlock);
                        move_node(G, nodeToBlock, to_block, other_block, block_weights, moved_out_of_separator, queues, rollback_info);
                } else {
                        NodeID nodeOtherBlock = queues[other_block].maxElement();
                        if( other_gain >= 0 && block_weights[other_block] + G.getNodeWeight(nodeOtherBlock) < config.upper_bound_partition) {
                                queues[other_block].deleteMax();
                                queues[to_block].deleteNode(nodeOtherBlock);
                                move_node(G, nodeOtherBlock, other_block, to_block, block_weights, moved_out_of_separator, queues, rollback_info);
                        } else {
                                // need to make progress (remove a random node from the queues)
                                if( nodeOtherBlock == nodeToBlock ) {
                                        queues[0].deleteMax();
                                        queues[1].deleteMax();
                                } else {
                                        int block = random_functions::nextInt(0,1);
                                        queues[block].deleteMax();
                                }
                        }
                }

                if( block_weights[2] <= best_separator ) {
                        best_separator = block_weights[2];
                        undo_idx = rollback_info.size();
                        std::cout <<  "undo idx " <<  undo_idx  << std::endl;
                } 

                if( queues[0].empty() ) {
                        break;
                } else {
                        gainToA = queues[0].maxValue();
                }

                if( queues[1].empty() ) {
                        break;
                } else {
                        gainToB = queues[1].maxValue();
                }

        }

         
        std::cout <<  "undo idx " <<  undo_idx << " roll_backinfo " <<  rollback_info.size()  << std::endl;
                  
        return 0;

}

