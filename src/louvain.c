#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "bipartite.h"
#include "tools.h"
#include "graph.h"
#include "modules.h"


/**
Louvain's algorithm of community detection for bipartite graphs. 

The Louvain's method is a graph hierarchical clustering algorithm by
optimization of a modularity function. It was first described in 2008
by Blondel et al. using Newman's modularity. This method do not
guarantee to reach the global optimum, but happens to be a powerful
heuristic to treat very large graphs.

This function is an implementation of the first step of this method
for Guimera's bipartite modularity function (Guimera et al. 2007).  It
is designed as a drop-in replacement for the SACommunityIdentBipart
function defined in bipartite.c.

@param binet The bipartite network to analyse. 
@param epsilon Threshold under which the modularity gain is not considered
significant anymore.
**/
struct group * LOUVCommunityIdentBipart(struct binet *binet, double epsilon)
{
  double gain = epsilon;
  int gain_this_loop = 0;
  int nagent;
  int gmax;
  double links=0, fac1=0, fac2=0, loss=0, ng=0;
  double **cooc;
  int *degree=NULL;
  struct node_gra *agents = binet->net1;
  struct node_gra *focal, *node;
  struct node_lis *neigh, *member;
  struct group *part = NULL;
  struct group **glist;
  struct node_gra *projected = NULL;

  // Project the agent network
  
  projected = ProjectBipart(binet);
  // Create the partition.
  part = CreateHeaderGroup();

  // Assgin each node to a group
  nnod = CountNodes(projected);
  ResetNetGroup(projected);
  focal = projected;
  glist = (struct group **) calloc(nnod, sizeof(struct group *));
  while ((focal = focal->next) != NULL) {
	glist[focal->num] = CreateGroup(part, focal->num);
	//printf ("Create group %d\n",glist[focal->num]->label);
	AddNodeToGroup(glist[focal->num], focal);
	//printf ("Node %d is in group %d\n",focal->num,focal->inGroup);
  } 


  //// COMPUTE NETWORK CONSTANTS
  //printf ("Compute network constant...\n");
  // Denominators from degree of teams.
  focal = binet->net2;
  while ((focal = focal->next) != NULL) {
	links = (double)CountLinks(focal);
	fac1 += links * (links - 1);
	fac2 += links;
  }
  fac1 = 1. / fac1; // 1/sum_a[m_a(m_a-1)]
  fac2 = 1. / fac2; // 1/sum_a[m_a]

  // Normalised matrix of concurrences and vector of degrees of
  // agents.
  cooc = allocate_d_mat(nnod, nnod);
  degree = allocate_i_vec(nnod);
  focal = agents;
  while ((focal = focal->next) != NULL) {
	degree[focal->num] = fac2 * CountLinks(focal);
	//printf ("node %d has %d links\n", focal->num, CountLinks(focal));
	node = agents;
	while ((node = node->next) != NULL)
	  cooc[focal->num][node->num] = fac1 * (double)NCommonLinksBipart(focal, node);
  }

  //// MAIN LOOP TROUGH THE NODES.
  //printf ("Main loop...\n");
  //printf ("gain %f\n",gain);
  focal = projected->next;
  while(1){
	//printf ("-------\n");
	//printf ("Node: %d, group: %d\n", focal->num, focal->inGroup);
	gain = .5 * epsilon; //because we are only computing 0.5 Delta M
	gmax = -1;

	neigh = focal->neig;
	while ((neigh = neigh->next) != NULL)
	  //printf ("Neighbor %d, %d, in group %d\n",neigh->node, neigh->ref->num, neigh->ref->inGroup);
	member = glist[focal->inGroup]->nodeList;
	while ((member = member->next) != NULL)
	  //printf ("Member %d, in group %d\n",member->ref->num, member->ref->inGroup);


	// modularity loss if the focal node is removed from its group.
	//printf ("Modularity loss\n");
	loss = 0;
	member = glist[focal->inGroup]->nodeList;
	while ((member = member->next) != NULL){
	  if (focal->num != member->node)
		loss -= cooc[focal->num][member->node] - degree[member->node] * degree[focal->num];
	}
	
	// Loop through the neighbors and compute potential modularity
	// gain.
	//printf ("Modularity gain \n");
	neigh = focal->neig;
	while ((neigh = neigh->next) != NULL) {
	  //printf ("Neighbor %d, %d, in group %d\n",neigh->node, neigh->ref->num, neigh->ref->inGroup);
	  if (neigh->ref->inGroup != focal->inGroup){
		ng = loss;
		member = glist[neigh->ref->inGroup]->nodeList;
		while ((member = member->next) != NULL){
		  //printf ("Member %d, in group %d\n",member->ref->num, member->ref->inGroup);
		  ng += cooc[focal->num][member->node] - degree[member->node] * degree[focal->num];
		}
		// If the gain is greater than the best potential gain.
		if (ng>gain){
		  gmax = neigh->ref->inGroup;
		  gain = ng;
		}
	  }
	}

	//printf ("gmax: %d, gain: %f\n",gmax,gain);
	// Change the group if there is a modularity increasing move.
	if (gmax != -1){
	  MoveNodeFast(focal,glist[focal->inGroup],glist[gmax]);
	  gain_this_loop = 1;
	}
	
	//printf ("get to next node\n");
	focal = focal->next;
	// If we get to the end and moved at least one node, loop back.
	if (focal == NULL){
	  if (gain_this_loop==0)
		break;
	  gain_this_loop = 0;
	  focal = projected->next;
	}
  } /// END MAIN LOOP TROUGH THE NODES.

  // Free memory
  free_d_mat(cooc,nnod);
  free_i_vec(degree);
  free(glist);

  return CompressPart(part);
}
