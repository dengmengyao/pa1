#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#define BAD_EXP -1111

enum {
  TK_NOTYPE = 256, TK_EQ, NUM, ADD, MINUS, MULTIPLY, DIVIDE, LBRACKET, RBRACKET, REG, HEX,AND, OR, NEQ

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  //词法的定义，定义各种符号
  {" +", TK_NOTYPE},    // spaces
  {"\\+", ADD},         // plus
  {"==", TK_EQ} ,        // equal
  {"0[xX][0-9a-fA-F]+",HEX},  //hexNumber
  {"[0-9]+",NUM},    //number
  {"\\-",MINUS},  //numus
  {"\\*",MULTIPLY},
  {"\\/",DIVIDE},
  {"\\(",LBRACKET},
  {"\\)",RBRACKET},
  {"\\$e[abcd]x",REG}, //register
  {"\\$e[bs]p",REG},
  {"\\$e[sd]i",REG},
  {"\\$eip",REG},
  {"&&",AND},
  {"\\|\\|",OR},
  {"!=",NEQ}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        //完善运算符功能
        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_NOTYPE:
             break;
          case NUM:
          case REG:
          case HEX:
              for(i=0;i<substr_len;i++)
                  tokens[nr_token].str[i]=substr_start[i];
              tokens[nr_token].str[i]='\0';
              nr_token++;
              break;
          case ADD:
              tokens[nr_token].str[0]=substr_start[0];
              tokens[nr_token++].str[1]='\0';
              break;
          case MINUS:
          case MULTIPLY:
              tokens[nr_token].str[0]=substr_start[0];
              tokens[nr_token++].str[1]='\0';
              break;
          case DIVIDE:
          case LBRACKET:
          case RBRACKET:
              tokens[nr_token].str[0]=substr_start[0];
              tokens[nr_token++].str[1]='\0';
              break;
          case AND:
          case OR:
          case TK_EQ:
          case NEQ:
              tokens[nr_token].str[0]=substr_start[0];
              tokens[nr_token].str[1]=substr_start[1];
              tokens[nr_token++].str[2]='\0';
              break;
          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

uint32_t expr(char *e, bool *success);
uint32_t eval(int p, int q);


uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */
  int i,cnt=0;
  bool judge=true;
  for(i=0;i<=nr_token;i++){
    if(tokens[i].type==LBRACKET)
       cnt++;
    else if(tokens[i].type==RBRACKET)
       cnt--;
    if(cnt<0)
       judge=false;
  }

  if(!judge)
     *success=false;
  else
     return eval(0,nr_token-1);
  return 0;
}

bool check_parenthese(int p,int q){//括号匹配函数
  int i,counter=0;
  for(i=p;i<=q;i++){//判断左右括号是否匹配
    if(tokens[i].type==LBRACKET)
      counter++;
    if(tokens[i].type==RBRACKET)
      counter--;
    if(counter==0&&i<q)
      return false;
  }
  if(counter!=0)
     return false;
  return true;
}

int dominant_operator(int p,int q){//寻找主操作运算符
  int i=0,j,cnt;
  int op=0,opp,pos=-1;
  for(i=p;i<=q;i++){
    if(tokens[i].type==NUM||tokens[i].type==REG||tokens[i].type==HEX)
       continue;
    else if(tokens[i].type==LBRACKET){
      cnt=0;
      for(j=i+1;j<=q;j++){
        if(tokens[j].type==RBRACKET){
          cnt++;
          i+=cnt;
          break;
        }
        else  
          cnt++;
      }
    }
    else{
      if(tokens[i].type==ADD||tokens[i].type==MINUS)//运算符优先级判断
        opp=2;
      else if(tokens[i].type==MULTIPLY||tokens[i].type==DIVIDE)
        opp=1;
      else if(tokens[i].type==OR)
        opp=5;
      else if(tokens[i].type==AND)
        opp=4;
      else if(tokens[i].type==NEQ||tokens[i].type==TK_EQ) 
        opp=3;
      else
        opp=0;
      if(opp>=op){
        pos=i;
        op=opp;
      }
    }
  }
  return pos;
}

uint32_t eval(int p,int q){
  if(p>q)
    return BAD_EXP;
  else if(p==q){//该位置是数值，判断数值类型后返回
    if(tokens[p].type==NUM)
       return atoi(tokens[p].str);
    else if(tokens[p].type==HEX){
      int cnt,i,len,sum=0;
      len=strlen(tokens[p].str);
      cnt=1;
      for(i=len-1;i>=0;i--){
        int num=0;
        if(tokens[p].str[i]>='0'&&tokens[p].str[i]<='9')
            num= tokens[p].str[i]-'0';
        else if(tokens[p].str[i]>='a'&&tokens[p].str[i]<='f')
            num=tokens[p].str[i]-'a'+10;
        else if(tokens[p].str[i]>='A'&&tokens[p].str[i]<='F')
            num= tokens[p].str[i]-'A'+10;
        sum=sum+cnt*num;
        cnt*=16;
      }
      return sum;
    }
    else if (tokens[p].type==REG){
      if(strcmp(tokens[p].str,"$eax")==0)
          return cpu.eax;
      else if(strcmp(tokens[p].str,"$ebx")==0)
          return cpu.ebx;
      else if(strcmp(tokens[p].str,"$ecx")==0)
          return cpu.ecx;
      else if (strcmp(tokens[p].str, "$edx") == 0)  
          return cpu.edx;
      else if (strcmp(tokens[p].str, "$ebp") == 0)  
          return cpu.ebp;
      else if (strcmp(tokens[p].str, "$esp") == 0)  
          return cpu.esp;
      else if (strcmp(tokens[p].str, "$esi") == 0)  
          return cpu.esi;
      else if (strcmp(tokens[p].str, "$edi") == 0)  
          return cpu.edi;
      else if (strcmp(tokens[p].str, "$eip") == 0)  
          return cpu.eip;
    }
  }
  else if(check_parenthese(p,q))//判断括号是否匹配
      return eval(p+1,q-1);
  else{//计算数值
    int op=dominant_operator(p,q);//判断主运算符
    uint32_t val1=eval(p,op-1);
    uint32_t val2=eval(op+1,q);
    switch(tokens[op].type){
      case ADD:
         return val1+val2;
      case MINUS:
         return val1-val2;
      case MULTIPLY:
         return val1*val2;
      case DIVIDE:
         return val1/val2;
      case AND:
         return val1&&val2;
      case OR:
         return val1||val2;
      case TK_EQ:
         return val1==val2;
      case NEQ:
         return val1!=val2;
      default:
         assert(0);
    }
  }
  return 1;
}
