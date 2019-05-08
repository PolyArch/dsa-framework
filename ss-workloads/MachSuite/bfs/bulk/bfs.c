/*
Implementations based on:
Harish and Narayanan. "Accelerating large graph algorithms on the GPU using CUDA." HiPC, 2007.
Hong, Oguntebi, Olukotun. "Efficient Parallel Graph Exploration on Multi-Core CPU and GPU." PACT, 2011.
*/

#include "bfs.h"

void bfs(node_t nodes[N_NODES], edge_t edges[N_EDGES],
            node_index_t starting_node, level_t level[N_NODES],
            edge_index_t level_counts[N_LEVELS])
{
  node_index_t n;
  edge_index_t e;
  level_t horizon;
  edge_index_t cnt;

  level[starting_node] = 0;
  level_counts[0] = 1;


  //int seen[N_NODES]={0};
  //int flush_before[N_NODES]={0};

  //for( n=0; n<N_NODES; n++ ) {
  //  edge_index_t tmp_begin = nodes[n].edge_begin;
  //  edge_index_t tmp_end = nodes[n].edge_end;
  //  int size = tmp_end-tmp_begin;

  //  //printf("node %d is on horizon, ",n);
  //  //printf("with %d edges \n",size);
 
  //  int seen_one = 0;
  //  printf("n%d: ",n);
  //  for( e=tmp_begin; e<tmp_end; e++ ) {
  //    node_index_t tmp_dst = edges[e].dst;
  //    if(seen[tmp_dst]) {
  //      seen_one=1;
  //      printf("**");
  //    }
  //    printf("%d ", tmp_dst);
  //  }
  //  printf("\n");
  // 
  //  if(seen_one) {
  //     for(int i = 0; i < N_NODES; ++i) {seen[i]=0;}   
  //     flush_before[n]=1;
  //  } 

  //  for( e=tmp_begin; e<tmp_end; e++ ) {
  //    node_index_t tmp_dst = edges[e].dst;
  //    seen[tmp_dst]=1;
  //  }
  //}

  //for(int i = 0; i < N_NODES; ++i) {
  //  if(flush_before[i]) {
  //    printf("%d ",i);
  //  }
  //}
  //printf("\n");


  loop_horizons: for( horizon=0; horizon<N_LEVELS; horizon++ ) {
    cnt = 0;
    //printf("********************     horizon %d\n",horizon);

    // Add unmarked neighbors of the current horizon to the next horizon
    loop_nodes: for( n=0; n<N_NODES; n++ ) {
      if( level[n]==horizon ) {
        edge_index_t tmp_begin = nodes[n].edge_begin;
        edge_index_t tmp_end = nodes[n].edge_end;
        int size = tmp_end-tmp_begin;

        //printf("node %d is on horizon, ",n);
        //printf("with %d edges \n",size);

        loop_neighbors: for( e=tmp_begin; e<tmp_end; e++ ) {
          node_index_t tmp_dst = edges[e].dst;
          level_t tmp_level = level[tmp_dst];

          //printf("temp_level %d  ", tmp_level);

          if( tmp_level ==MAX_LEVEL ) { // Unmarked
            level[tmp_dst] = horizon+1;
            ++cnt;
          } else {
            level[tmp_dst] = tmp_level;
          }

          //printf("edge[%d]",e);
          //printf("\tlevel[%d]=%d\n",tmp_dst,level[tmp_dst]);
        }
        //printf("------------\n");

        //for( e=tmp_begin; e<tmp_end; e++ ) {
        //  printf("%d ", level[edges[e].dst]);
        //}
        //printf("\n");
      }
    }

    //for(int i = 0; i < N_NODES; ++i) {
    //  printf("%d ",level[i]);
    //}
    //printf("\n");

    //printf("cnt =%d\n",cnt);

    if( (level_counts[horizon+1]=cnt)==0 )
      break;
  }
}
