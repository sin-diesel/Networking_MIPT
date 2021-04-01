


struct field {
	int m;
	int (*sum) (int a, int b);
};

int register_field(struct field* f);

