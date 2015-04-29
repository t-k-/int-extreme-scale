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

int cnt_unfree = 0;

void prog_dep_add(struct prog *pg0, struct prog *pg1)
{
	struct dep_ptr *save, **p = &pg0->dp;
	save = *p;
	*p = malloc(sizeof(struct dep_ptr));
	cnt_unfree ++;
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
		cnt_unfree ++;
		found_a->name = a;
		found_a->dp = NULL;
		prog_dep_add(root, found_a);
	} 

	found_a->stage = S_NOT_READY;

	if (found_b == NULL) {
		found_b = malloc(sizeof(struct prog));
		cnt_unfree ++;
		found_b->name = b;
		found_b->stage = S_READY;
		found_b->dp = NULL;
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
		if (p->pg)
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

void rm_dep(struct prog *root, struct prog *rm)
{
	struct dep_ptr *q, *p = root->dp;
	struct dep_ptr **prev;

	while (p != NULL) {
		q = p->pg->dp;
		prev = &p->pg->dp;
		while (q != NULL) {
			if (q->pg == rm) {
				*prev = q->next;
//				printf("delete dependency %c from %c...\n", 
//						q->pg->name, p->pg->name);
				free(q);
				cnt_unfree --;
				break;
			}

			prev = &q->next;
			q = q->next;
		}

		p = p->next;
	}
}

int rm_one_without_dep(struct prog *root)
{
	struct dep_ptr *p = root->dp;
	struct dep_ptr **prev = &root->dp;
	int ret = 0;

	while (p != NULL) {
		if (p->pg->dp == NULL) {
			*prev = p->next;
			rm_dep(root, p->pg);
			//printf("delete %c...\n", p->pg->name);
			free(p->pg);
			free(p);
			cnt_unfree -= 2;
			ret = 1;
			break;
		}

		prev = &p->next;
		p = p->next;
	}

	return ret;
}

int loop_test(struct prog *root)
{
	do {
		;
		//print_all_dependencies(root);
	} while (rm_one_without_dep(root));

	if (root->dp != NULL) {
		printf("there is a loop:\n");
		print_all_dependencies(root);
		return 1;
	}

	return 0;
}

int free_all_dep(struct prog *prog, int free_prog)
{
	struct dep_ptr *save;
	while (prog->dp) {
		save = prog->dp;
		prog->dp = prog->dp->next;
		if (free_prog) {
			free(save->pg);
			cnt_unfree --;
		}
		free(save);
		cnt_unfree --;
	}
}

int free_all(struct prog *root)
{
	struct dep_ptr *p = root->dp;
	while (p != NULL) {
		free_all_dep(p->pg, 0);
		p = p->next;
	}

	free_all_dep(root, 1);
}

int main(int argc, char* argv[])
{
	struct prog root = {'\0', 0, NULL};
	unsigned int limit = LIMIT_INFTY;
	char conf_file[] = "dep.cfg";
	char line_buff[4096];
	int loop_exist;
	int read_again;
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

	read_again = 0;
read_again:

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

	if (!read_again) {
		loop_exist = loop_test(&root);

		if (loop_exist) {
			goto exit;
		} else {
			read_again = 1;
			printf("read again...\n");
			goto read_again;
		}
	}

	while (res)
		res = run_ready_programs(&root, limit);
		
exit:
	free_all(&root);
	printf("unfree: %d\n", cnt_unfree);
	return 0;
}
