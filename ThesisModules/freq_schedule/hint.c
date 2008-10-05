
int hint[TOTAL_STATES] = {0};

void clear_hint(void)
{
	int i;
	for(i=0;i<TOTAL_STATES;i++)
		hint[i] = 0;
}
EXPORT_GPL(clear_hint);

int hint_count(void)
{
	int i,count=0;
	for(i=0;i<TOTAL_STATES;i++)
		if(hint[i] > 0)
			count++;
	return count;
}
EXPORT_GPL(hint_count);


