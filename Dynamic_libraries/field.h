

extern int sum(int m, int a, int b);


struct field {
	int m;
	int (*sum) (int a, int b);
};

extern int register_field(struct field* f);

