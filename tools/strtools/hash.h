
typedef struct _node{
  char *name;
  char *desc;
  struct _node *next;
}node;

void inithashtab();
unsigned int hash(char *s);
node* lookup(char *n);
char* m_strdup(char *o);
char* get(char* name);
int install(char* name,char* desc);
void displaytable();
void cleanup();
