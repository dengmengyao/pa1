#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

WP* new_wp();
void free_wp(WP *wp);

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){//返回空监视点给head链表，将表达式值赋值给监视点
  if(free_==NULL)
     assert(0);
  WP *wp=free_;
  free_=free_->next;
  wp->next=NULL;

  return wp;
}

void free_wp(WP *wp){//将wp所指的结点归还到free_链表中
  wp->exp[0]='\0';
  wp->value=-1;
  wp->next=free_;
  free_=wp;
}

void insert_wp(char *args) {//向链表中插入新的监视点
  bool flag = true;
  uint32_t val = expr(args, &flag);
  if (!flag) {
    printf("You input an invalid expression, failed to create watchpoint!");
    return ;
  }  
  WP *wp = new_wp();
  wp->value = val;
  strcpy(wp->exp, args);
  if (head != NULL) {//为空时插入首位
    WP *wwp;
    wwp = head;
    while (wwp->next != NULL) {
      wwp = wwp->next;
    }
    wp->NO = wwp->NO + 1;
    wwp->next = wp;    
  }
  else {//链表不为空则从后面插入监视点
    wp->NO = 1;
    head = wp;
  }
  return;
}

void delete_wp(int no) {//删除监视点
  if (head == NULL) {
    printf("There is no watchpoint to delete!");
    return;
  }
  WP *wp;
  if (head->NO != no) {//输出无法找到监视点的原因
    wp = head;
    while (wp->next != NULL && wp->next->NO != no) {
      wp = wp->next;
    }
    if (wp != NULL) {//监视点已被删除
      WP *delete_wp;
      delete_wp = wp->next;
      wp->next = delete_wp->next;
      free_wp(delete_wp);
      printf("NO.%d  watchpoint has been deleted!\n", no);
    }
    else {//无法找到监视点
      printf("Failed to find the NO.%d watchpoint!", no);
    }

  }
  else {//找到目标监视点后释放对应点
    wp = head;
    head=head->next;
    free_wp(wp);
  }
  return ;
}

void print_wp() {//打印现在所有的监视点
  if (head == NULL) {
    printf("There is no watchpoint!\n");
    return ;
  }
  WP *wp;
  printf("NO      expression        value\n");
  wp = head;
  while (wp != NULL) {//按链表顺序打印监视点
    printf("%-5d   %-15s   %-16u\n", wp->NO, wp->exp, wp->value);
    wp = wp->next;
  }
}

int * test_change(){
  WP *wp=head;
  bool flag=true;
  uint32_t val;
  static int no[NR_WP];
  int i=0;
  while(wp!=NULL){
    val=expr(wp->exp,&flag);
    if(val!=wp->value){
      wp->value=val;
      no[i++]=wp->NO;
    }
    wp=wp->next;
  }
  no[i]=-1;
  return no;
}
