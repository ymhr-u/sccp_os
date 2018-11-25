#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STRMAX 80
#define MEMSIZE 64

/*
  it is assumed that the unit of memory allocation is 1KB
  and the total memory size is 64KB.
*/

typedef struct node {
  int segment;  /* segment number (use -1 for a hole) */
  int begin; /* beginning address (in allocation unit: 1KB) */
  int size; /* segment size (in allocation unit) */
  struct node *prev, *next;
} node;


node *head;    /* pointers to the first node  in the linked list */
int segno; /* counter indicating the segment number for the next allocation */
int debug = 0; /* if != 0, mirros each input line to stdout */
int n_alloc, n_dealloc; /* allocate(), deallocate() の実行回数 */
int total_alloc_size ; /* allocate() した領域の積算サイズ */
int n_traverse; /* allocate() でリンクをたどった回数の積算 */
int alloc_flag;  /*ここが1の状態でallocate()を実行するとbest_fitを実行*/
int total_req_size;// (alloc()   で要求された穴のサイズの積算)
int non_exist_seg; // (dealloc で指定されたセグメントが存在しない場合の数)
int not_enough_mem;//(compactionしてもallocできない場合の数)
int n_compaction; //(何回compactionしたか？)
int total_move_mem; //(compactionで移動させたメモリーサイズの積算)
int over_size; //空き以上に割り当ててしまった回数
int total_hole_size = MEMSIZE;
int dflag; //デバッグ用変数
int bflag;

void allocate(int);
void print_list();
void print_stat();
void deallocate(int);
void merge(node*);
void compact();
node *bestfit(int);
node *firstfit(int);

int main(int argc, char **argv){

  if((argc >1) && (strcmp(argv[1], "-d") == 0)){dflag = 1;}
  if((argc >1) && (strcmp(argv[1], "-b") == 0)){bflag = 1;}

  int  a;
  char c;
  char line[80], cmd[80];
  segno = 0;

  head = (node *)malloc(sizeof(node));
  head->next = NULL;
  head->prev = NULL;
  head->size = MEMSIZE;
  head->begin= 0;
  head->segment = -1;


  //  while(scanf(" %c",&c) != EOF){
  while(fgets(line, STRMAX, stdin) != NULL){
    if(sscanf(line, "%s %d", cmd,&a) > 0){
      switch(cmd[0]){

      case 'a':
	if(bflag){
	  printf("allocate %dKB (best fit)\n",a);
	  alloc_flag = 1;
	  if(a<=0){printf("allocate size error\n");}
	  else{allocate(a);}
	  a = -1;
	  break;
	}
	else{
	  printf("allocate %dKB (first fit)\n",a);
	  alloc_flag =0;
	  if(a<=0){printf("allocate size error\n");}
	  else{allocate(a);}
	  a = -1;
	  break;
	}

	/*      case 'b':
        printf("allocate %dKB (best fit)\n",a);
	alloc_flag = 1;
	if(a<=0){printf("allocate size error\n");}
	else{allocate(a);}
	a = -1;
	break;
	*/
      case 'd':
	printf("deallocate:SegNo.%d\n",a);
      	if(a<0 || a> segno-1){printf("%d does not exist and cannot be deallocated.\n",a);}
	else{deallocate(a);}
	break;

      case 'p':
	if(!dflag){print_list();}
	break;

     case 'c':
	compact();
	break;

      default:
	printf("Command Error:this is not a valid command.\n");
	break;
      }
    }

    if(dflag){
      printf("\n");
      print_stat();
    }
    printf("\ncommand:");
  }
  printf("\n");
  print_stat();
  exit(EXIT_SUCCESS);
}


void allocate(int size){

  total_req_size+=size;

  struct node *new=(node *)malloc(sizeof(node));
  n_alloc++;
  struct node *x;
  if(alloc_flag){
    x = bestfit(size);
    if(x==NULL){
      over_size++;
      return;
    }
  }
  else{
    x= firstfit(size);
    if(x==NULL){
      over_size++;
      return;
    }
  }

  total_alloc_size += x->size;
  if(x->size != size){
    if(x->next!=NULL){x->next->prev=new;}
    new->begin = x->begin+size ;
    new->segment=-1;
    new->size=x->size-size;
    new->prev  = x;
    new->next  = x->next;
    x->next= new;
    x->segment  = segno++;
    x->size = size;

  }else{
    /* request size == hole size*/
    free(new);
    x->segment = segno++;
  }
  print_list();

}

void print_list(){
  struct node *cur = head;

  int i=0;
  printf("  ");
  while(cur!=NULL){
    if(cur->segment == -1 ){printf("H");}
    else{printf("P");}

    printf("(");
    if(cur->segment != -1)printf("%d ",cur->segment);
    printf("%d %d) ",cur->begin,cur->size);
    cur=cur->next;
  }

  printf("\n");
  cur = head;

  printf("|");
  while(cur!=NULL){
    if(cur->segment == -1 ){
      for(i=0;i<cur->size;i++){printf("H");}
    }else{
      for(i=0;i<cur->size;i++){printf("P");}
    }
    printf("|");
    cur=cur->next;

  }
  printf("\n");

  return;
}


void print_stat(){
  printf("No of traverse: %.1d\n",n_traverse);
  printf("No of allocation:%d\n", n_alloc);
  printf("Total allocationSize:%d\n", total_alloc_size);
  printf("Average Allocation Size: %.1f\n",(float)total_alloc_size/(float)n_alloc);
  printf("Total  moving: %.1d\n",total_move_mem);
  printf("Total  request:%d\n",total_req_size);
  printf("Number of compaction: %d\n\n",n_compaction);
}

void deallocate(int num_seg){
  int flag=1;
  node *tmp_hole;
  struct node *cur = head;
  n_dealloc++;

  while(cur!=NULL){
    if(cur->segment == num_seg){
      total_hole_size+=cur->size;
      tmp_hole = cur;
      cur->segment = -1;
      flag--;
      break;
    }
    cur = cur->next;
  }
  if(flag){
    printf("Not found...seg:%d\n",num_seg);
    non_exist_seg++;
  }

  merge(tmp_hole);
  print_list();
}

void merge(node *target){

  struct node *p = head;

  while(p!=target){
    p=p->next;
  }

 /*head*/
  if(p == head){
    if(p->next->segment == -1){
      p->next->size+=p->size;
      head =p->next;
    }
  }
  /*tail*/
  else if(p->next == NULL){
    if(p->prev->segment == -1){
      p->prev->size += p->size;
      p->prev->next = p->next;
    }
  }

  else{
    //case(d) : H P H
    if(p->next->segment==-1 && p->prev->segment == -1){
      p->prev->size += p->size;
      p->prev->size += p->next->size;
      if(p->next->next!=NULL){
	p->prev->next = p->next->next;
	p->next->next->prev = p->prev;
      }else{
	p->prev->next =NULL;
      }
    }
    //case(b) : P H H
    else if( p->next->segment == -1){
      p->size += p->next->size;
      if(p->next->next!=NULL){
	p->next = p ->next ->next;
	p->next->prev=p;
      }else{
	p->next=NULL;
      }
    }
    //case(c) : H H P
    else if(p->prev->segment == -1){
      p->prev->size += p->size;
      p->prev->next = p->next;
      p->next->prev = p->prev;
    }
    //case(a) : nothing to do
    else{}
  }
  return;
}



node *firstfit(int request){
  struct node *cur = head;
  int biggest=0;

  if(request>total_hole_size){
    not_enough_mem++;
    printf("Can't allocate this size\n");
    return NULL;
  }

  while(cur!=NULL){
    if(cur->segment == -1){
      total_hole_size+=cur->size;
      if(cur->size >= request){
	total_hole_size-=cur->size;
	return cur;
      }
      if(cur->size > biggest)biggest = cur->size;
    }
    n_traverse++;
    cur = cur->next;
  }


    printf("\nExecute compaction\n");
    compact();
    n_compaction++;
    printf("\nAllocate again\n");
    return firstfit(request);

}

node *bestfit(int request){
  struct node *cur = head,*return_node;
  int biggest=0,flag=0;

  return_node=(node *)malloc(sizeof(node));
  return_node->size = 99999;

  if(request>total_hole_size){
    not_enough_mem++;
    printf("Can't allocate this size\n");
    return NULL;
  }

  while(cur!=NULL){
    if(cur->segment == -1){
      total_hole_size+=cur->size;
      if(cur->size >= request && cur->size < return_node->size ){
	return_node=cur;
	flag = 1;
      }
      if(cur->size > biggest)biggest = cur->size;
    }
    n_traverse++;
    cur = cur->next;
  }

  if(flag){
    total_hole_size-=return_node->size;
    return return_node;
  }

  printf("\nExecute compaction\n");
  compact();
  n_compaction++;
  printf("\nAllocate again\n");
  return bestfit(request);
}


void compact(){
  struct node *cur = head;
  int emp_size=0,i=0,j,k;
  int cal[100];
  int before_begin,after_begin;
  while(cur!=NULL){
    if(cur->segment == -1){
      cal[i]=cur->size;
      emp_size+=cur->size;
      if(cur==head){head=cur->next;}
      else if(cur->next ==NULL ){cur->prev->next=NULL;}//tail
      else{
	cur->prev->next = cur->next;
	cur->next->prev = cur->prev;
      }
      i++;
    }
    cur = cur->next;
  }

  for(j=0;j<i;j++){
    total_move_mem +=64;
    for(k=0;k<i;k++){
      total_move_mem -= cal[k];
    }
  }

  cur = head;
  while(cur->next!=NULL){cur = cur->next;}

  cur->begin = cur->prev->begin + cur->prev->size;

  struct node *new=(node *)malloc(sizeof(node));
  new->prev  = cur;
  new->next  = NULL;
  new->begin = cur->begin + cur->size;
  new->segment=-1;
  new->size=emp_size;

  cur->next= new;
  print_list();
}
