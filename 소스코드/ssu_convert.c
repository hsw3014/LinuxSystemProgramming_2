#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"convert_start.h"
#include"ssu_gettimeofday.h"

char* fname = NULL;

int main(int argc, char* argv[])
{
	int opt;		//옵션설정 변수들
	int o_j = 0;
	int o_c = 0;
	int o_p = 0;
	int o_f = 0;
	int o_l = 0;
	int o_r = 0;

	if(argc < 2){
		fprintf(stderr, "use parameter 2 more\n");
		exit(1);
	}

	fname = argv[1];	//파일명 기억시킴

	while((opt = getopt(argc, argv, "jcpflr")) != -1){
		switch(opt)
		{
			case 'j' : o_j++; break;
			case 'c' : o_c++; break;
			case 'p' : o_p++; break;
			case 'f' : o_f++; break;
			case 'l' : o_l++; break;
			case 'r' : o_r++; break;
			default : fprintf(stderr, "Error for Option. Not Valuable\n");
					  exit(1);;
		}
		if(o_j > 1 || o_c > 1 || o_p > 1 || o_f > 1 || o_l > 1 || o_r > 1){
			fprintf(stderr, "Used same option more twice.\n");
			exit(1);
		}
	}
	gettimeofday(&begin_t, NULL);
	convert_start(o_j, o_c, o_p, o_f, o_l, o_r);
	gettimeofday(&end_t, NULL);
	ssu_runtime(&begin_t, &end_t);
}
