#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* getopt */

#include <stdint.h>
#define LIMIT_INFTY UINT32_MAX

struct prog;

struct dep_ptr {
	struct prog *pg;
	struct dep_ptr *next;
};

enum stage_num {
	S_NOT_READY,
	S_READY,
	S_EXECUTED
};

struct prog {
	char name;
	enum stage_num stage;
	struct dep_ptr *dp;
};

void prog_dep_add(struct prog *pg0, struct prog *pg1)
{
	struct dep_ptr *save, **p = &pg0->dp;
	save = *p;
	*p = malloc(sizeof(struct dep_ptr));
	(*p)->pg = pg1;
	(*p)->next = save;
}

void prog_dep_pri(struct prog *pg)
{
	struct dep_ptr *p = pg->dp;
	
	while (p != NULL) {
		printf("%c ", p->pg->name);
		p = p->next;
	}
	printf("\n");
}

void add_dependency(struct prog *root, char a, char b)
{
	struct dep_ptr *p = root->dp;
	struct prog *found_a = NULL, *found_b = NULL;

	while (p != NULL) {
		if (p->pg->name == a) {
			found_a = p->pg;
		}

		if (p->pg->name == b) {
			found_b = p->pg;
		}

		if (found_a && found_b)
			break;

		p = p->next;
	}

	if (found_a == NULL) {
		found_a = malloc(sizeof(struct prog));
		found_a->name = a;
		prog_dep_add(root, found_a);
	} 

	found_a->stage = S_NOT_READY;

	if (found_b == NULL) {
		found_b = malloc(sizeof(struct prog));
		found_b->name = b;
		found_b->stage = S_READY;
		prog_dep_add(root, found_b);
	} 

	prog_dep_add(found_a, found_b);
}

void print_all_dependencies(struct prog *root)
{
	struct dep_ptr *p = root->dp;
	char *comment;
	while (p != NULL) {
		switch (p->pg->stage) {
		case S_READY:
			comment = "ready";
			break;
		case S_NOT_READY:
			comment = "not ready";
			break;
		default:
			comment = "executed";
		}
		printf("for program %c (%s): ", 
		       p->pg->name, comment);
		prog_dep_pri(p->pg);
		p = p->next;
	}
}

int run_ready_programs(struct prog *root, unsigned int limit)
{
	struct dep_ptr *p, *q;
	int flag, ret = 0;
	unsigned int cnt = 0;

	p = root->dp;
	while (p != NULL) {
		if (p->pg->stage == S_READY) {
			printf("%c ", p->pg->name);
			p->pg->stage = S_EXECUTED;
			ret = 1;
			cnt ++;
		}

		if (cnt == limit)
			break;

		p = p->next;
	}

	if (ret)
		printf("\n");
	else
		return ret;
	
	p = root->dp;
	while (p != NULL) {
		if (p->pg->stage == S_NOT_READY) {
			q = p->pg->dp;
			flag = 1;
			while (q != NULL) {
				if (q->pg->stage != S_EXECUTED) {
					flag = 0;
					break;
				}
				q = q->next;
			}

			if (flag)
				p->pg->stage = 1;
		}
		p = p->next;
	}

	return ret;
}

int test()
{
	struct prog root = {'\0', 0, NULL};
	int res;

	add_dependency(&root, 'b', 'a');
	add_dependency(&root, 'c', 'a');
	add_dependency(&root, 'd', 'a');
	add_dependency(&root, 'e', 'b');
	add_dependency(&root, 'e', 'c');
	add_dependency(&root, 'e', 'd');

	do {
		print_all_dependencies(&root);
		res = run_ready_programs(&root, LIMIT_INFTY);
	} while (res);
		
	return 0;
}

int main(int argc, char* argv[])
{
	struct prog root = {'\0', 0, NULL};
	unsigned int limit = LIMIT_INFTY;
	char conf_file[] = "dep.cfg";
	char line_buff[4096];
	int res = 1;
	char a, b;
	FILE *fh;
	int c;

	while ((c = getopt(argc, argv, "l:")) != -1) {
        switch (c) {
        case 'l':
            sscanf(optarg, "%u", &limit);
			if (limit == 0)
				limit = 1;
			printf("set limit = %u\n", limit);
            break;
        default:
			printf("bad argument(s).");
			return 0;
        }
    }

	fh = fopen(conf_file, "r");

	if (fh == NULL) {
		printf("configuration file not found.\n");
		return 0;
	}

	while (fgets (line_buff, sizeof(line_buff), fh)) {
		res = sscanf(line_buff, "%c %c", &a, &b);
		printf("add: %c depends on %c\n", a, b);
		add_dependency(&root, a, b);
	}

	fclose(fh);

	while (res)
		res = run_ready_programs(&root, limit);
		
	return 0;
}
