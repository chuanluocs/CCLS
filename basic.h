#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

/* limits on the size of the problem. */
//#define MAX_VARS    10000010
//#define MAX_CLAUSES 43000043

#define pop(stack) stack[--stack ## _fill_pointer]
#define push(item, stack) stack[stack ## _fill_pointer++] = item

// Define a data structure for a literal in the SAT problem.
struct lit {
	int clause_num;		//clause num, begin with 0
	int var_num;		//variable num, begin with 1
	int sense;			//is 1 for true literals, 0 for false literals.
};

enum PROBLEMTYPE {NONE, WEIGHTED, UNWEIGHTED, WEIGHTED_PARTIAL};
enum PROBLEMTYPE probtype;


/*parameters of the instance*/
int num_vars;		//var index from 1 to num_vars
int num_clauses;		//clause index from 0 to num_clauses-1

long long hard_clause_weight;

int	maxi_clause_len;
int	mini_clause_len;
long long maxi_clause_weight;
long long mini_clause_weight;
const long long diff_crafted_weight = 800;

/* literal arrays */				
lit** var_lit;				//var_lit[i][j] means the j'th literal of var i.
int* var_lit_count;			//amount of literals of each var
lit** clause_lit;			//clause_lit[i][j] means the j'th literal of clause i.
int* clause_lit_count; 			// amount of literals in each clause			
long long* clause_weight;	
			
/* Information about the variables. */
long long* score;				
int* conf_change;
int** var_neighbor;
int* var_neighbor_count;
int* neighbor_flag;

/* Information about the clauses */					
int* sat_count;			
int* sat_var;			

//unsat clauses stack
int* unsat_stack;		//store the unsat clause number
int	unsat_stack_fill_pointer;
int* index_in_unsat_stack;//which position is a clause in the unsat_stack

//variables in unsat clauses
int* unsatvar_stack;		
int	unsatvar_stack_fill_pointer;
int* index_in_unsatvar_stack;
int* unsat_app_count;		//a varible appears in how many unsat clauses


/* Information about solution */
int* cur_soln;	//the current solution, with 1's for True variables, and 0's for False variables
int* best_soln;

int* best_array;
int best_array_count;

//cutoff steps
const unsigned long long max_tries = 9223372036854775806ll;
const unsigned long long max_flips = 9223372036854775806ll;
unsigned long long total_unsat_clause_weight=0ll;
unsigned long long total_clause_weight=0ll;
double opt_time;
unsigned long long opt_step;
unsigned long long opt_unsat_clause_weight;
//int is_opt_unsat_clause_weight_first_assigned;
long long opt_unsat_clause_count;
long long total_soft_clause_weight=0ll;


int* temp_lit;
int* temp_neighbor;
int temp_neighbor_count;
ifstream infile;

void build_instance_weighted();
void build_instance_unweighted();
void build_instance_weighted_partial();

void allocate_memory()
{
	int malloc_var_length = num_vars+10;
	int malloc_clause_length = num_clauses+10;
	
	var_lit = new lit* [malloc_var_length];
	var_lit_count = new int [malloc_var_length];
	score = new long long [malloc_var_length];
	conf_change = new int [malloc_var_length];
	var_neighbor = new int* [malloc_var_length];
	var_neighbor_count = new int [malloc_var_length];
	neighbor_flag = new int [malloc_var_length];
	unsatvar_stack = new int [malloc_var_length];
	index_in_unsatvar_stack = new int [malloc_var_length];
	unsat_app_count = new int [malloc_var_length];
	cur_soln = new int [malloc_var_length];
	best_soln = new int [malloc_var_length];
	best_array = new int [malloc_var_length];
	temp_lit = new int [malloc_var_length];
	temp_neighbor = new int [malloc_var_length];
	
	clause_lit = new lit* [malloc_clause_length];
	clause_lit_count = new int [malloc_clause_length];
	clause_weight = new long long [malloc_clause_length];
	sat_count = new int [malloc_clause_length];
	sat_var = new int [malloc_clause_length];
	unsat_stack = new int [malloc_clause_length];
	index_in_unsat_stack = new int [malloc_clause_length];
}

void free_memory()
{
	int v, c;
	
	for(v=1; v<=num_vars; v++) 
	{
		delete [] var_lit[v];
		delete [] var_neighbor[v];
	}
	delete [] var_lit;
	delete [] var_lit_count;
	delete [] score;
	delete [] conf_change;
	delete [] var_neighbor;
	delete [] var_neighbor_count;
	delete [] neighbor_flag;
	delete [] unsatvar_stack;
	delete [] index_in_unsatvar_stack;
	delete [] unsat_app_count;
	delete [] cur_soln;
	delete [] best_soln;
	delete [] best_array;
	delete [] temp_lit;
	delete [] temp_neighbor;
	
	for(c=0; c<num_clauses; c++)
	{
		delete [] clause_lit[c];
	}
	delete [] clause_lit;
	delete [] clause_lit_count;
	delete [] clause_weight;
	delete [] sat_count;
	delete [] sat_var;
	delete [] unsat_stack;
	delete [] index_in_unsat_stack;
}

void build_neighbor_relation()
{
	int		i,j,count;
	int 	v,c;

	for(v=1; v<=num_vars; ++v)
	{
		neighbor_flag[v] = 0;
	}
	neighbor_flag[0] = 0;

	for(v=1; v<=num_vars; ++v)
	{
		var_neighbor_count[v] = 0;
		//for(i=1; i<=num_vars; ++i)
		//	neighbor_flag[i] = 0;
		neighbor_flag[v] = 1;
		temp_neighbor_count = 0;
		for(i=0; i<var_lit_count[v]; ++i)
		{
			c = var_lit[v][i].clause_num;
			for(j=0; j<clause_lit_count[c]; ++j)
			{
				if(neighbor_flag[clause_lit[c][j].var_num]==0)
				{
					var_neighbor_count[v]++;
					neighbor_flag[clause_lit[c][j].var_num] = 1;
					temp_neighbor[temp_neighbor_count++] = clause_lit[c][j].var_num;
				}
			}
			
		}

		neighbor_flag[v] = 0;
 
		var_neighbor[v] = new int[var_neighbor_count[v]+1];

		count = 0;
		for(i=0; i<temp_neighbor_count; i++)
		{
			var_neighbor[v][count++] = temp_neighbor[i];
			neighbor_flag[temp_neighbor[i]] = 0;
		}
		/*
		for(i=1; i<=num_vars; ++i)
		{
			if(neighbor_flag[i]==1)
			{
				var_neighbor[v][count] = i;
				count++;
			}
		}
		*/
		var_neighbor[v][count]=0;
		var_neighbor_count[v] = count;
	}
}


int build_instance(char *filename)
{
	//char            line[1024];
	//char            tempstr1[10];
	//char            tempstr2[10];
	
	string line, tempstr1, tempstr2;
	const char* c_line;
	int             i,v,c;
	string		check_is_partial;
	
	infile.open(filename);
	if(infile==NULL)
		return 0;

	/*** build problem data structures of the instance ***/
	//infile.getline(line,1024);
	//while (line[0] != 'p')
	//	infile.getline(line,1024);

	//sscanf(line, "%s %s %d %d", tempstr1, tempstr2, &num_vars, &num_clauses);
	
	getline(infile, line);
	c_line = line.c_str();
	while(c_line[0] != 'p')
	{
		getline(infile, line);
		c_line = line.c_str();
	}
	
	istringstream input_line(line);
	check_is_partial = "";
	input_line >> tempstr1 >> tempstr2 >> num_vars >> num_clauses >> check_is_partial;
	
	allocate_memory();
	
	/*
	if(num_vars>=MAX_VARS)
	{
		printf("c the number of variables exceeds MAX_VARS, please enlarge MAX_VARS.\n");
		fflush(stdout);
		exit(-1);
	}
	if( num_clauses>=MAX_CLAUSES)
	{
		printf("c the number of clauses exceeds MAX_CLAUSES, please enlarge MAX_CLAUSES.\n");
		fflush(stdout);
		exit(-1);
	}
	*/
	
	for (c = 0; c < num_clauses; c++) 
		clause_lit_count[c] = 0;
	for (v=1; v<=num_vars; ++v)
		var_lit_count[v] = 0;
	
	maxi_clause_len = -1;
	mini_clause_len = num_vars+1;
	maxi_clause_weight = -1;
	mini_clause_weight = -1;
	//Now, read the clauses, one at a time.
	
	if(check_is_partial.compare("")==0)
	{
		if(strcmp(tempstr2.c_str(),"wcnf")==0)
			build_instance_weighted();
		else build_instance_unweighted();
	}
	else
	{
		istringstream myisstream(check_is_partial);
		myisstream >> hard_clause_weight;
		build_instance_weighted_partial();
	}
	
	infile.close();
	
	//creat var literal arrays
	for (v=1; v<=num_vars; ++v)
	{
		var_lit[v] = new lit[var_lit_count[v]+1];
		var_lit[v][var_lit_count[v]].var_num = 0;
		var_lit[v][var_lit_count[v]].clause_num=-1;
		var_lit_count[v] = 0;	//reset to 0, for build up the array
	}
	//scan all clauses to build up var literal arrays
	for (c = 0; c < num_clauses; ++c) 
	{
		for(i=0; i<clause_lit_count[c]; ++i)
		{
			v = clause_lit[c][i].var_num;
			var_lit[v][var_lit_count[v]] = clause_lit[c][i];
			++var_lit_count[v];
		}
	}
	
	return 1;
		
}

void build_instance_weighted()
{
	//char            line[1024];
	//char            tempstr1[10];
	//char            tempstr2[10];
	int             cur_lit;
	int             i,c;
	
	int lit_redundent,clause_redundent;
	
	probtype = WEIGHTED;
	
	for (c = 0; c < num_clauses; ) 
	{
		clause_redundent=0;
		
		infile>>clause_weight[c];
		total_clause_weight+=(unsigned long long)clause_weight[c];
		infile>>cur_lit;
		while (cur_lit != 0) { 
		
			lit_redundent=0;
			for(int p=0; p<clause_lit_count[c]; p++)
			{
				if(cur_lit==temp_lit[p]){
					lit_redundent=1;
					break;
				}
				else if(cur_lit==-temp_lit[p]){
					clause_redundent=1;
					break;
				}
			}
			
			if(lit_redundent==0)
			{
		
				temp_lit[clause_lit_count[c]] = cur_lit;
				clause_lit_count[c]++;
			}
		
			infile>>cur_lit;
		}
		
		if(clause_redundent==0)
		{
			clause_lit[c] = new lit[clause_lit_count[c]+1];
		
			for(i=0; i<clause_lit_count[c]; ++i)
			{
				clause_lit[c][i].clause_num = c;
				clause_lit[c][i].var_num = abs(temp_lit[i]);
				if (temp_lit[i] > 0) clause_lit[c][i].sense = 1;
					else clause_lit[c][i].sense = 0;
			
				var_lit_count[clause_lit[c][i].var_num]++;
			}
			clause_lit[c][clause_lit_count[c]].var_num=0;
			clause_lit[c][clause_lit_count[c]].clause_num=-1;
			
			maxi_clause_len = maxi_clause_len>clause_lit_count[c]?maxi_clause_len:clause_lit_count[c];
			mini_clause_len = mini_clause_len<clause_lit_count[c]?mini_clause_len:clause_lit_count[c];
			if(maxi_clause_weight==-1)
				maxi_clause_weight = clause_weight[c];
			else maxi_clause_weight = maxi_clause_weight>clause_weight[c]?maxi_clause_weight:clause_weight[c];
			if(mini_clause_weight==-1)
				mini_clause_weight = clause_weight[c];
			else mini_clause_weight = mini_clause_weight<clause_weight[c]?mini_clause_weight:clause_weight[c];
			
			c++;
		}
		else
		{
			num_clauses--;
			clause_lit_count[c] = 0;
		}
	}
}

void build_instance_unweighted()
{
	//char            line[1024];
	//char            tempstr1[10];
	//char            tempstr2[10];
	int             cur_lit;
	int             i,c;
	
	int lit_redundent,clause_redundent;
	
	probtype = UNWEIGHTED;
	
	for (c = 0; c < num_clauses; ) 
	{
		clause_redundent=0;
		
		clause_weight[c] = 1;
		total_clause_weight+=(unsigned long long)clause_weight[c];
		infile>>cur_lit;
		while (cur_lit != 0) { 
		
			lit_redundent=0;
			for(int p=0; p<clause_lit_count[c]; p++)
			{
				if(cur_lit==temp_lit[p]){
					lit_redundent=1;
					break;
				}
				else if(cur_lit==-temp_lit[p]){
					clause_redundent=1;
					break;
				}
			}
			
			if(lit_redundent==0)
			{
		
				temp_lit[clause_lit_count[c]] = cur_lit;
				clause_lit_count[c]++;
			}
		
			infile>>cur_lit;
		}
		
		if(clause_redundent==0)
		{
			clause_lit[c] = new lit[clause_lit_count[c]+1];
		
			for(i=0; i<clause_lit_count[c]; ++i)
			{
				clause_lit[c][i].clause_num = c;
				clause_lit[c][i].var_num = abs(temp_lit[i]);
				if (temp_lit[i] > 0) clause_lit[c][i].sense = 1;
					else clause_lit[c][i].sense = 0;
			
				var_lit_count[clause_lit[c][i].var_num]++;
			}
			clause_lit[c][clause_lit_count[c]].var_num=0;
			clause_lit[c][clause_lit_count[c]].clause_num=-1;
			
			maxi_clause_len = maxi_clause_len>clause_lit_count[c]?maxi_clause_len:clause_lit_count[c];
			mini_clause_len = mini_clause_len<clause_lit_count[c]?mini_clause_len:clause_lit_count[c];
			if(maxi_clause_weight==-1)
				maxi_clause_weight = clause_weight[c];
			else maxi_clause_weight = maxi_clause_weight>clause_weight[c]?maxi_clause_weight:clause_weight[c];
			if(mini_clause_weight==-1)
				mini_clause_weight = clause_weight[c];
			else mini_clause_weight = mini_clause_weight<clause_weight[c]?mini_clause_weight:clause_weight[c];
			
			c++;
		}
		else
		{
			num_clauses--;
			clause_lit_count[c] = 0;
		}
	}
}

void build_instance_weighted_partial()
{
	//char            line[1024];
	//char            tempstr1[10];
	//char            tempstr2[10];
	int             cur_lit;
	int             i,c;
	
	int lit_redundent,clause_redundent;
	
	probtype = WEIGHTED_PARTIAL;
	
	total_soft_clause_weight = 0ll;
	
	for (c = 0; c < num_clauses; ) 
	{
		clause_redundent=0;
		
		infile>>clause_weight[c];
		total_clause_weight+=(unsigned long long)clause_weight[c];
		infile>>cur_lit;
		while (cur_lit != 0) { 
		
			lit_redundent=0;
			for(int p=0; p<clause_lit_count[c]; p++)
			{
				if(cur_lit==temp_lit[p]){
					lit_redundent=1;
					break;
				}
				else if(cur_lit==-temp_lit[p]){
					clause_redundent=1;
					break;
				}
			}
			
			if(lit_redundent==0)
			{
		
				temp_lit[clause_lit_count[c]] = cur_lit;
				clause_lit_count[c]++;
			}
		
			infile>>cur_lit;
		}
		
		if(clause_redundent==0)
		{
			clause_lit[c] = new lit[clause_lit_count[c]+1];
		
			for(i=0; i<clause_lit_count[c]; ++i)
			{
				clause_lit[c][i].clause_num = c;
				clause_lit[c][i].var_num = abs(temp_lit[i]);
				if (temp_lit[i] > 0) clause_lit[c][i].sense = 1;
					else clause_lit[c][i].sense = 0;
			
				var_lit_count[clause_lit[c][i].var_num]++;
			}
			clause_lit[c][clause_lit_count[c]].var_num=0;
			clause_lit[c][clause_lit_count[c]].clause_num=-1;
			
			maxi_clause_len = maxi_clause_len>clause_lit_count[c]?maxi_clause_len:clause_lit_count[c];
			mini_clause_len = mini_clause_len<clause_lit_count[c]?mini_clause_len:clause_lit_count[c];
			if(maxi_clause_weight==-1)
				maxi_clause_weight = clause_weight[c];
			else maxi_clause_weight = maxi_clause_weight>clause_weight[c]?maxi_clause_weight:clause_weight[c];
			if(mini_clause_weight==-1)
				mini_clause_weight = clause_weight[c];
			else mini_clause_weight = mini_clause_weight<clause_weight[c]?mini_clause_weight:clause_weight[c];
			
			if(clause_weight[c]<hard_clause_weight) total_soft_clause_weight+=clause_weight[c];
			
			c++;
		}
		else
		{
			num_clauses--;
			clause_lit_count[c] = 0;
		}
	}
	
	for(c=0; c<num_clauses; c++)
	{
		if(clause_weight[c] == hard_clause_weight) clause_weight[c] = total_soft_clause_weight+1;
	}
	hard_clause_weight = total_soft_clause_weight+1;
}


inline void unsat(int clause)
{
	int v;
	index_in_unsat_stack[clause] = unsat_stack_fill_pointer;
	push(clause,unsat_stack);
	
	total_unsat_clause_weight += (unsigned long long)clause_weight[clause];
	lit * p = clause_lit[clause];

	for(;(v=p->var_num)!=0; p++)
	{
		unsat_app_count[v]++;
		if(unsat_app_count[v]==1)
		{
			index_in_unsatvar_stack[v] = unsatvar_stack_fill_pointer;
			push(v,unsatvar_stack);	
		}
	}
}


inline void sat(int clause)
{
	int index,last_unsat_clause,v,last_unsat_var;

	//since the clause is satisfied, its position can be reused to store the last_unsat_clause
	last_unsat_clause = pop(unsat_stack);
	index = index_in_unsat_stack[clause];
	unsat_stack[index] = last_unsat_clause;
	index_in_unsat_stack[last_unsat_clause] = index;
	
	total_unsat_clause_weight -= (unsigned long long)clause_weight[clause];
	lit * p = clause_lit[clause];
	
	for(;(v=p->var_num)!=0; p++)
	{
		unsat_app_count[v]--;
		if(unsat_app_count[v]==0)
		{
			last_unsat_var = pop(unsatvar_stack);
			index = index_in_unsatvar_stack[v];
			unsatvar_stack[index] = last_unsat_var;
			index_in_unsatvar_stack[last_unsat_var] = index;
		}
	}
}

//initiation of the algorithm
void init()
{
	int 		v,c;
	int			i,j;
	
	//init unsat_stack
	unsat_stack_fill_pointer = 0;
	unsatvar_stack_fill_pointer = 0;
	total_unsat_clause_weight = 0ll;
	
	build_neighbor_relation();

	//init solution
	for (v = 1; v <= num_vars; v++) {

		if(rand()%2==1) cur_soln[v] = 1;
		else cur_soln[v] = 0;
		conf_change[v] = 1;
		unsat_app_count[v]=0;
	}

	/* figure out sat_count, and init unsat_stack */
	for (c=0; c<num_clauses; ++c) 
	{
		sat_count[c] = 0;
		
		for(j=0; j<clause_lit_count[c]; ++j)
		{
			if (cur_soln[clause_lit[c][j].var_num] == clause_lit[c][j].sense)
			{
				sat_count[c]++;
				sat_var[c] = clause_lit[c][j].var_num;	
			}
		}

		if (sat_count[c] == 0) 
		{
			unsat(c);
		}
	}

	/*figure out var dscore*/
	int lit_count;
	for (v=1; v<=num_vars; v++) 
	{
		score[v] = 0;
		
		lit_count = var_lit_count[v];
		
		for(i=0; i<lit_count; ++i)
		{
			c = var_lit[v][i].clause_num;
			if (sat_count[c]==0) score[v]+=clause_weight[c];
			else if (sat_count[c]==1 && var_lit[v][i].sense==cur_soln[v]) score[v]-=clause_weight[c];
		}
	}
	
	
	//setting for the virtual var 0
	conf_change[0]=0;
	score[0]=0;
}

 
//flip a var, and do the necessary updating
void flip(int flipvar)
{
	int v,c;
	lit* clause_c;
	lit* p;
	lit* q;

	cur_soln[flipvar] = 1 - cur_soln[flipvar];
	
	//update related clauses and neighbor vars
	for(q=var_lit[flipvar]; (c=q->clause_num)!=-1; q++)
	{
		clause_c = clause_lit[c];
		if(cur_soln[flipvar] == q->sense)
		{
			++sat_count[c];
			
			if (sat_count[c] == 2) //sat_count from 1 to 2
				score[sat_var[c]]+=clause_weight[c];
			else if (sat_count[c] == 1) // sat_count from 0 to 1
			{
				sat_var[c] = flipvar;//record the only true lit's var
				score[flipvar]-=clause_weight[c];
				
				for(p=clause_c; (v=p->var_num)!=0; p++)
				{
					score[v]-=clause_weight[c];
				}

				sat(c);
			}
		}
		else // cur_soln[flipvar] != cur_lit.sense
		{
			--sat_count[c];
			if (sat_count[c] == 1) //sat_count from 2 to 1
			{
				for(p=clause_c; (v=p->var_num)!=0; p++)
				{
					if(p->sense == cur_soln[v] )
					{
						score[v] -=clause_weight[c];
						sat_var[c] = v;
						break;
					}
				}
			}
			else if (sat_count[c] == 0) //sat_count from 1 to 0
			{
				for(p=clause_c; (v=p->var_num)!=0; p++)
				{
					score[v] +=clause_weight[c];
				}
				score[flipvar]+=clause_weight[c];
				unsat(c);
			}//end else if
			
		}//end else
	}
	
	int * np = var_neighbor[flipvar];
	for(; (v=*np)!=0; np++)
	{
		conf_change[v] = 1;
	}
	//update information of flipvar
	conf_change[flipvar] = 0;
}


/*the following functions are non-algorithmic*/

void print_solution()
{
     printf("v");
     for (int i=1; i<=num_vars; i++) {
		printf(" ");
		if(best_soln[i]==0) printf("-");
		printf("%d", i);
     }
     printf("\n");
     fflush(stdout);
}

int verify_sol_non_partial()
{
	int c,j,flag;
	unsigned long long verify_weights=0ll;
	for (c = 0; c<num_clauses; ++c) 
	{
		flag = 0;
		for(j=0; j<clause_lit_count[c]; ++j)
			if (best_soln[clause_lit[c][j].var_num] == clause_lit[c][j].sense) {flag = 1; break;}

		if(flag ==0){//output the clause unsatisfied by the solution
			verify_weights += clause_weight[c];
		}
	}
	if(verify_weights==opt_unsat_clause_weight)
		return 1;
	return 0;
}

int verify_sol_partial()
{
	int c,j,flag;
	unsigned long long verify_weights=0ll;
	for (c = 0; c<num_clauses; ++c) 
	{
		flag = 0;
		for(j=0; j<clause_lit_count[c]; ++j)
			if (best_soln[clause_lit[c][j].var_num] == clause_lit[c][j].sense) {flag = 1; break;}

		if(flag ==0){//output the clause unsatisfied by the solution
			verify_weights += clause_weight[c];
		}
	}
	if(verify_weights==opt_unsat_clause_weight && opt_unsat_clause_weight<hard_clause_weight)
		return 1;
	return 0;
}


