#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<sys/wait.h>
#include"convert_table.h"
#include"header_table.h"

extern char* fname;
char where[MAX][BUFFER_SIZE];
int where_count = 1;
int indent_count = 0;	//해당횟수만큼 탭
int delete_brace = 0;	//이 값에 해당하는 }는 출력되지 않음.
int brace_indent[MAX] = {0};	//} 맞춤용 카운트
int brace_count = 1;
int for_if_indent = 0;
int before_add_indent = 0;
int construct_on = 0;

char define_space[MAX][BUFFER_SIZE];	//define으로 추가되어야할 변수들
int define_count = 0;
char define_var[MAX][BUFFER_SIZE];
int define_var_count = 0;
char extern_space[MAX][BUFFER_SIZE];
int extern_count = 0;
char deleted_name[MAX];
int p_opt_check[CON_FN] = {0};

char r_opt_java[MAX][BUFFER_SIZE] = {0};
char r_opt_buf[MAX][BUFFER_SIZE] = {0};
char line_buf[BUFFER_SIZE] = {0};
int r_java_count = 0;
int r_opt_count = 0;
int hang_java = 1;
int hang_c = 1;
int before_where = 1;
int brace_check = 0;

void reset_buf(char* imsy_buf);
void reset_token(char(*token_list)[BUFFER_SIZE], int len);
void token_delete(char(*token_list)[BUFFER_SIZE], int len, char* delimiter);

void convert_start(int o_j, int o_c, int o_p, int o_f, int o_l, int o_r)
{
	int i,j;
	int fd;
	int fd_list[MAX];
	int fd_count = 0;
	int status;
	pid_t pid = -1;
	before_where = where_count;

	int token_length;

	char t;
	char imsy_buf[BUFFER_SIZE];
	char all_buf[BUFFER_SIZE];
	char filename[MAX];
	strcpy(filename, fname);
	char* token_fname = strtok(filename, ".");

	if(o_r){	//r옵션 - 자식프로세스 생성
		if((pid = fork()) < 0){
			fprintf(stderr, "fork error\n");
			exit(1);
		}

		if(pid > 0){	//부모라면 기다려서 자식이 모두 처리토록 하고 끝냄
			while(!waitpid(pid, &status, WNOHANG)){
				//계속 자식끝날때까지 대기함.
			}
			exit(0);
		}
	}

	if((fd = open(fname, O_RDONLY)) < 0){
		fprintf(stderr, "Not exists File : %s\n", fname);
		exit(1);
	}

	while(read(fd, &t, 1) > 0){
		strncat(all_buf, &t, 1);
		if(t == '\t')
			continue;

		if(t == '}'){
			if(indent_count == delete_brace)	//삭제가일어났던 시점의 indent부분은 스킵함
				continue;
		}

		if(t == '\n'){	//처리 시작
			//토큰분리
			exit_header_need[fd_count] = 1;
			for_if_indent = 0;
			strncat(imsy_buf, &t, 1);
			char result_list[BUFFER_SIZE] = {0};
			int Match = 0;

			//printf("BEFORE : %s\n",imsy_buf);
			token_length = set_token(imsy_buf, token_buf);
			strcpy(token_buf[token_length],"\n");
			//전처리기 부분 처리
			///////////////////////////
			
			if(construct_on == 0){
				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], where[fd_count]) == 0){
						if(i < token_length -1){
							if(strcmp(token_buf[i+1], "(") == 0){	//이름( 일경우
								construct_on = 1;
								Match = 1;
							}
						}
					}
				}
			}
			
			if(construct_on == 1){	//함수형태로 변환
				strcpy(token_buf[0], "void");
				char extern_imsy_buf[BUFFER_SIZE] = {0};
				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "{") == 0 || strcmp(token_buf[i], "\n") == 0)
						break;
					strcat(extern_imsy_buf, token_buf[i]);
				}
				strcpy(extern_space[extern_count++], extern_imsy_buf);
				construct_on = -1;
			}

			if(construct_on == 0 && fd_count > 1 && strlen(where[fd_count-1]) > 0){	//삭제될이름 정함
				char* checkname = where[fd_count-1];
				char* ptr = strstr(imsy_buf, checkname);
				if(ptr != NULL){
					Match = 1;
					construct_on = -1;	//삭제된 이름은 한번만 쓰임(생성자일때만)
					for(j=0; j<token_length; j++){
						if(strcmp(token_buf[j], checkname) == 0){
							strcpy(token_buf[j], "");
							strcpy(deleted_name, token_buf[j+2]);
							break;
						}
					}

					for(j=0; j<token_length; j++){
						if(strcmp(token_buf[j], checkname) == 0){
							break;
						}
						strcpy(token_buf[j], "");
					}
				}
			}

			if(strlen(deleted_name))	//삭제될 이름이 존재할 경우에만
			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], deleted_name) == 0){
					Match = 1;
					strcpy(token_buf[i], "");
					strcpy(token_buf[i+1], "");
					i++;
				}
			}

			if(strcmp(token_buf[0], "}") == 0 && token_length == 2){ //토큰이 }뿐이라면
				indent_count--;		// }가 추가됨에따라 인덴트와 brace_count도 감소
				brace_count--;

				brace_check = 1;
			}

			//토큰리스트와 기존 리스트를 이용해 처리를 시작할것.

			int i,j;
			int l_len = strlen(imsy_buf);
			char imsy_list[BUFFER_SIZE] = {0};
			char* ptr = NULL;

			//리스트에서 strstr으로 1차확인

			ptr = strstr(imsy_buf, main_keyword);
			if(ptr != NULL){
				Match = 1;
				int offset = 1;
				for(i=0; i<strlen(imsy_buf); i++){
					if(imsy_buf[i] == ';')
						offset = i;
				}

				strcat(result_list, "int main(void)");		//메인으로변환
				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "{") == 0){	//토큰비움
						if(strcmp(token_buf[i+1],"\n") == 0)
							strcpy(token_buf[i+1], "");
						break;
					}
					strcpy(token_buf[i], "");
				}
				ptr = strstr(ptr + offset, main_keyword);
			}

			ptr = strstr(imsy_buf, print_keyword);
			while(ptr != NULL){	//printf로 변경해야함

				for(i=0; i<token_length; i++){
					if(i+5 < token_length){	//길이 예외처리 (System, ., out, ., printf)
						reset_buf(imsy_list);	//버퍼비우고

						for(j=i; j<i+5; j++){	//채운뒤 비교
							strcat(imsy_list, token_buf[j]);
						}

						if(strcmp(imsy_list, print_keyword) == 0){
							//System.out.printf 발견시
							Match = 1;
							p_opt_check[0] = 1;	//p옵션을 위한 저장
							IO_header_need[fd_count] = 1;
							int plen = strlen(print_keyword);

							strcat(result_list, "printf");
							for(j=i+5; j<token_length; j++){
								strcat(result_list, token_buf[j]);
							}
							//strcat(result_list, ptr+plen);

							for(j=i; j<token_length; j++){
								if(strcmp(token_buf[j], ";") == 0){
									strcpy(token_buf[j], "");
									if(strcmp(token_buf[j+1], "\n") == 0)
										strcpy(token_buf[j+1], "");
									break;
								}
								strcpy(token_buf[j], "");\
							}
						}
					}
				}

				ptr = strstr(ptr+1, print_keyword);
			}

			reset_buf(imsy_list);
			
			//Scanner
			ptr = strstr(imsy_buf, Scann_keyword);
			while(ptr != NULL){	// ; 까지 전부날려야함
				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], Scanner_keyword) == 0){
						//Scanner토큰이 발견됬을 경우
						if(i < token_length-3){
							char check_semi = token_buf[i+3][0];
							if(check_semi == ';'){	//Scanner ???; 형태
								//아무것도 담아가지않음
								for(j=i; j<=i+3; i++){
									strcpy(token_buf[i], "");
									if(strcmp(token_buf[j+1], "\n") == 0)
										strcpy(token_buf[j+1], "");
								}
								Match = 1;
							}
						}
					}

					token_delete(token_buf, token_length, ";");
					Match = 1;
				}
				ptr = strstr(ptr+1, Scann_keyword);
			}

			reset_buf(imsy_list);

			ptr = strstr(imsy_buf, scanf_keyword);
			while(ptr != NULL){	//scanf로 변경해야함
				for(i=0; i<token_length; i++){
					if(i+10 < token_length){	//공백포함 최대길이는 10
						reset_buf(imsy_list);

						for(j=i; j<i+10; j++){
							strcat(imsy_list, token_buf[j]);
							if(strcmp(token_buf[j], ";") == 0)
								break;
						}

						if(strstr(imsy_list, scanf_keyword) != NULL){
							//담아온 부분에서 발견되면
							char var[MAX];
							char changebuf[BUFFER_SIZE] = {0};

							strcpy(var, token_buf[i]);	//가장 앞의것이 변수로 저장
							sprintf(changebuf, "scanf(\"%%d\", &%s);\n", var);
							strcat(result_list, changebuf);

							for(j=i; j<i+10; j++){	//변형된부분은 제거
								if(strcmp(token_buf[j], ";") == 0){
									strcpy(token_buf[j], "");
									break;
								}
								strcpy(token_buf[j], "");
							}
							Match = 1;
							p_opt_check[1] = 1;
							IO_header_need[fd_count] = 1;	//헤더 추가
						}
					}
				}
				ptr = strstr(ptr+1, scanf_keyword);
			}

			reset_buf(imsy_list);
			
			//new file
			ptr = strstr(imsy_buf, file_keyword);
			while(ptr != NULL){	//char* 형에 담아야함
				char filename[MAX] = {0};
				char change_this[BUFFER_SIZE] = {0};
				char var_name[MAX] = {0};
				int start_offset;

				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "File") == 0){
						//토큰중 File토큰이 발견되면 다음토큰이 변수이름
						strcpy(var_name, token_buf[i+2]);
						start_offset = i;
						break;
					}
				}

				int ptrmove = strlen(file_keyword);
				ptr+=ptrmove;	//file(는 읽지않음
				while(*ptr != ')'){
					char c = *ptr;
					strcat(filename, &c);	//"~~" 를가지고옴
					ptr+=1;
				}
				sprintf(change_this, "char* %s = %s;\n", var_name, filename);
				strcat(result_list, change_this);

				for(i=start_offset; i<token_length; i++){
					strcpy(token_buf[i], "");	//변환되었으니 토큰 삭제
				}
				Match = 1;
				ptr = strstr(ptr+1, file_keyword);
			}
			
			//new Filewriter 처리
			ptr = strstr(imsy_buf, FileWriter_keyword);
			while(ptr != NULL){	//FileWriter write = new FileWriter 처리
				IO_header_need[fd_count] = 1;	//현재파일에 헤더 추가
				char fileds[MAX] = {0};
				char change_this[BUFFER_SIZE] = {0};
				char make_FILE[BUFFER_SIZE] = {0};
				int start_offset;

				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "FileWriter") == 0){
						strcpy(fileds, token_buf[i+2]);	//FILE* 변수
						start_offset = i;
						break;
					}
				}

				sprintf(make_FILE, "FILE* %s;\n", fileds);
				strcat(change_this, make_FILE);
				for(i=0; i<indent_count; i++) strcat(change_this, "\t");

				int ptrmove = strlen(FileWriter_keyword);
				ptr+=ptrmove;
				char* var_name = strtok(ptr, ",");
				char* truefalse = strtok(NULL, ")");
				char cpy_truefalse[BUFFER_SIZE] = {0};
				int tlen = strlen(truefalse);
				int cpy_count = 0;

				for(i=0; i<tlen; i++){
					if(truefalse[i] == ' ')	//공백제거
						continue;
					cpy_truefalse[cpy_count++] = truefalse[i];
				}

				if(strcmp(cpy_truefalse, "true") == 0){	//참이면 뒤부터 쓰여짐
					char make_buf[BUFFER_SIZE] = {0};
					sprintf(make_buf, "if((%s = fopen(%s, \"a\")) == NULL){\n\t", fileds, var_name);
					strcat(change_this, make_buf);
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "fprintf(stderr, \"fopen error\\n\");\n\t");
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "exit(1);\n");
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "}\n");
					strcat(result_list, change_this);
					Match = 1;
					p_opt_check[2] = 1;

					for(i=0; i<token_length; i++){
						if(strcmp(token_buf[i], ";") == 0){
							strcpy(token_buf[i], "");
							if(strcmp(token_buf[i+1], "\n") == 0)
								strcpy(token_buf[i+1], "");
							break;
						}
						strcpy(token_buf[i], "");
					}
				}
				else if(strcmp(cpy_truefalse, "false") == 0){	//거짓이면 처음부터 쓰여짐
					char make_buf[BUFFER_SIZE] = {0};
					sprintf(make_buf, "if((%s = fopen(%s, \"w\")) == NULL){\n\t", fileds, var_name);

					strcat(change_this, make_buf);
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "fprintf(stderr, \"fopen error\\n\");\n\t");
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "exit(1);\n");
					for(i=0; i<indent_count; i++) strcat(change_this, "\t");
					strcat(change_this, "}\n");
					strcat(result_list, change_this);
					Match = 1;
					p_opt_check[2] = 1;

					for(i=0; i<token_length; i++){
						if(strcmp(token_buf[i], ";") == 0){
							strcpy(token_buf[i], "");
							if(strcmp(token_buf[i+1], "\n") == 0)
								strcpy(token_buf[i+1], "");
							break;
						}
						strcpy(token_buf[i], "");
					}
				}
				ptr = strstr(ptr + 1, FileWriter_keyword);
			}

			ptr = strstr(imsy_buf, write_keyword);	//fwrite에맞게 변환
			while(ptr != NULL){
				//fwrite("~~", sizeof("~~"), 1, var);
				char change_this[BUFFER_SIZE] = {0};
				char var_name[BUFFER_SIZE] = {0};
				char w_data[BUFFER_SIZE] = {0};
				int wlen = strlen(write_keyword);
				int start_offset;
				ptr+=wlen;	//write( 부터 읽어들이기 시작함.
				while(*ptr != ')'){
					char c = *ptr;
					strcat(w_data, &c);
					ptr+=1;
				}	//w_data에 입력할 내용이 담겨지게된다.

				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "write") == 0){
						//토큰중 File토큰이 발견되면 전전토큰이 변수이름
						if(i>=2){
							if(strcmp(token_buf[i-1], ".") == 0){
								strcpy(var_name, token_buf[i-2]);
								start_offset = i-2;
								Match = 1;
								p_opt_check[3] = 1;
								break;
							}
						}
					}
				}

				if(Match){	//; 나올때까지 토큰제거
					for(i=start_offset; i<token_length; i++){
						if(strcmp(token_buf[i], ";") == 0){
							if(strcmp(token_buf[i+1], "\n") == 0)
								strcpy(token_buf[i+1], "");
							strcpy(token_buf[i], "");
							break;
						}
						strcpy(token_buf[i], "");
					}
				}

				sprintf(change_this, "fwrite(%s, sizeof(%s), 1, %s);\n",w_data, w_data, var_name);
				strcat(result_list, change_this);	//변환
				ptr = strstr(ptr + 1, write_keyword);
			}

			ptr = strstr(imsy_buf, flush_keyword);	//fflush로 변환
			while(ptr != NULL){
				char change_this[BUFFER_SIZE] = {0};
				char var_name[BUFFER_SIZE] = {0};
				int start_offset;

				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "flush") == 0){
						if(i>=2){
							if(strcmp(token_buf[i-1], ".") == 0){
								strcpy(var_name, token_buf[i-2]);	//변수이름 가지고옴
								start_offset = i-2;
								Match = 1;
								p_opt_check[4] = 1;
								break;
							}
						}
					}
				}

				for(i=start_offset; i<token_length; i++){
					if(strcmp(token_buf[i], ";") == 0){
						if(strcmp(token_buf[i+1], "\n") == 0)
							strcpy(token_buf[i+1], "");
						strcpy(token_buf[i], "");
						break;
					}
					strcpy(token_buf[i], "");
				}

				sprintf(change_this, "fflush(%s);\n", var_name);
				strcat(result_list, change_this);
				ptr = strstr(ptr + 1, flush_keyword);
			}
			
			//fclose
			ptr = strstr(imsy_buf, close_keyword);	//fclose로 변환
			while(ptr != NULL){
				char change_this[BUFFER_SIZE] = {0};
				char var_name[BUFFER_SIZE] = {0};
				int start_offset;

				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "close") == 0){
						if(i>=2){
							if(strcmp(token_buf[i-1], ".") == 0){
								strcpy(var_name, token_buf[i-2]);	//변수이름 가지고옴
								start_offset = i-2;
								Match = 1;
								p_opt_check[5] = 1;
								break;
							}
						}
					}
				}

				for(i=start_offset; i<token_length; i++){		//토큰지우기
					if(strcmp(token_buf[i], ";") == 0){
						if(strcmp(token_buf[i+1], "\n") == 0)
							strcpy(token_buf[i+1], "");
						strcpy(token_buf[i], "");
						break;
					}
					strcpy(token_buf[i], "");
				}

				sprintf(change_this, "fclose(%s);\n", var_name);
				strcat(result_list, change_this);
				ptr = strstr(ptr + 1, close_keyword);
			}
			

			//static
			if(Match == 0){
				ptr = strstr(imsy_buf, static_keyword);	//static 변수 처리
				while(ptr != NULL){
					Match = 1;
					int find_var;
					int offsetmove = 0;
					for(i=offsetmove; i<strlen(imsy_buf); i++){
						if(imsy_buf[i] == ';'){
							offsetmove = i;
							break;
						}
					}
					for(i=0; i<token_length; i++){
						find_var = 1;
	
						for(j=0; j<TYPE_N; j++){
							if(strcmp(token_buf[i], type_keyword[j]) == 0){
								find_var = 0;
								break;
							}
							if(strcmp(token_buf[i], " ") == 0){
								find_var = 0;
								break;
							}
						}
						if(find_var){	//define시킬 목록에 추가 및 토큰 비우기
							if(strcmp(token_buf[i+1], "=") == 0){//a= ???
								if(strcmp(token_buf[i+2], " ") == 0){// =옆이 공백이라면
									strcpy(define_var[define_var_count++], token_buf[i+3]);
									strcpy(define_space[define_count++], token_buf[i]);
									strcpy(token_buf[i], "");
									strcpy(token_buf[i+1], "");
									strcpy(token_buf[i+2], "");
									strcpy(token_buf[i+3], "");
									i+=3;
									continue;
								}
								else{	//a=???
									strcpy(define_var[define_var_count++], token_buf[i+2]);
									strcpy(define_space[define_count++], token_buf[i]);
									strcpy(token_buf[i], "");
									strcpy(token_buf[i+1], "");
									strcpy(token_buf[i+2], "");
									i+=2;
									continue;
								}
							}
							else if(strcmp(token_buf[i+2], "=") == 0){// a = ???꼴
								if(strcmp(token_buf[i+3], " ") == 0){
									strcpy(define_var[define_var_count++], token_buf[i+4]);
									strcpy(define_space[define_count++], token_buf[i]);
									strcpy(token_buf[i], "");
									strcpy(token_buf[i+1], "");
									strcpy(token_buf[i+2], "");
									strcpy(token_buf[i+3], "");
									strcpy(token_buf[i+4], "");
									i+=4;
									continue;
								}
								else{	//a =???
									strcpy(define_var[define_var_count++], token_buf[i+3]);
									strcpy(define_space[define_count++], token_buf[i]);
									strcpy(token_buf[i], "");
									strcpy(token_buf[i+1], "");
									strcpy(token_buf[i+2], "");
									strcpy(token_buf[i+3], "");
									i+=3;
									continue;
								}
							}
						}
						if(strcmp(token_buf[i],";") == 0){
							strcpy(token_buf[i], "");
							break;
						}
						strcpy(token_buf[i], "");
					}
					ptr = strstr(ptr + offsetmove, static_keyword);
				}
			}

			//java 특수형
			if(Match == 0){
				for(i=0; i<TYPE_E; i++){	//java 특수형일 경우 처리
					ptr = strstr(imsy_buf, except_type[i]);
					while(ptr != NULL){
						Match = 1;
						int offset = 1;
						for(j=0; j<strlen(imsy_buf); j++){
							if(imsy_buf[j] == ';'){
								offset = j;
							}
						}
						
						strcat(result_list, matching_type[i]);
						strcat(result_list, ptr + strlen(except_type[i]));
	
						token_delete(token_buf, token_length, ";");
	
						ptr = strstr(ptr + offset, except_type[i]);
					}
				}
			}
			
			if(Match == 0){
				for(i=0; i<TYPE_E; i++){
					ptr = strstr(imsy_buf, new_type[i]);
					while(ptr != NULL){
						Match = 1;
						int offset = 1;
						char var[MAX] = {0};
						char change_this[BUFFER_SIZE] = {0};

						for(j=0; j<strlen(imsy_buf); j++){
							if(imsy_buf[j] == ';'){
								offset = j;
							}
						}

						for(j=0; j<token_length; j++){
							if(strcmp(token_buf[j], "[") == 0){
								strcpy(var, token_buf[j+1]);	//변수 혹은 상수 담아옴
								break;
							}
						}
						
						for(j=0; j<token_length; j++){	//new 앞까지 전부 담음
							if(strcmp(token_buf[j], "new") == 0){
								break;
							}
							strcat(result_list, token_buf[j]);
						}

						sprintf(change_this, " %s(sizeof(%s));\n", new_matching_type[i], var);
						strcat(result_list, change_this);

						token_delete(token_buf, token_length, ";");

						ptr = strstr(ptr + offset, new_type[i]);
					}
				}
			}
			//TOKEN으로 체크	

			//for, if일때 1줄에 한하여 추가 indent
			for_if_indent = 0;
			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], for_keyword) == 0 || strcmp(token_buf[i], if_keyword) == 0)
					for_if_indent = 1;
			}

			//return 체크
			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], return_keyword) == 0){
					if(strcmp(where[where_count-1], token_fname) == 0){ //main일때만 exit변환
						char change[MAX] = {0};
						if(strcmp(token_buf[i+2],";") == 0 || strcmp(token_buf[i+1], ";"))
							sprintf(change, "exit(0);\n");
						else
							sprintf(change, "exit(%s);\n", token_buf[i+2]);

						strcat(result_list, change);

						for(j=i; j<token_length; j++){
							strcpy(token_buf[j], "");
						}
						Match = 1;
						exit_header_need[fd_count] = 1;
						break;
					}
				}
			}

			//import 체크
			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], imp_keyword) == 0){
					//import가 존재하니까 비워버림. 뒤를 더 볼 필요없음 ; 까지
					Match = 1;
					while(strcmp(token_buf[i], ";") != 0){	// ; 나올때까지 전부제거
						strcpy(token_buf[i++], "");
					}
				}
			}

			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], class_keyword) == 0){
					//class가 존재하니 다음토큰이 파일명으로 .c파일 생성, 해당라인 삭제 및 중괄호관리
					construct_on = 0;
					if(i != token_length-2){
						//다음토큰이름으로 파일이 생성되고 where에 추가됨
						char* newfile = token_buf[i+2];
						char newfilename[MAX];
						sprintf(newfilename, "%s.c", newfile);
						if((fd_list[fd_count++] = open(newfilename, O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0){
							fprintf(stderr, "Cannot open file %s\n", newfile);
							exit(0);
						}
						strcpy(where[where_count++], newfile);
					}
					Match = 1;
					delete_brace = indent_count;	//이 번호에 해당하는 }는 출력되지않음

					for(j=0; j<token_length; j++)
						strcpy(token_buf[j], "");
				}
			}

			//public 체크, NULL 변환
			int pcheck = 0;
			for(i=0; i<token_length; i++){
				if(strcmp(token_buf[i], null_keyword) == 0){
					Match = 1;
					strcpy(token_buf[i], "NULL");
				}
			}

			ptr = strstr(imsy_buf, pub_keyword);
			if(ptr != NULL){
				if(Match == 0){
					char extern_imsy_buf[MAX] = {0};
	
					for(i=0; i<token_length; i++){
						if(strcmp(token_buf[i], pub_keyword) == 0){
							//해당 public 토큰과 다음 공백 없앰
							Match = 1;
							strcpy(token_buf[i], "");
							strcpy(token_buf[i+1], "");
							i+=1;
							continue;
						}
	
						if(strcmp(token_buf[i], "{") == 0 || strcmp(token_buf[i], "\n") == 0)
							break;
						strcat(extern_imsy_buf, token_buf[i]);
					}
					strcpy(extern_space[extern_count++], extern_imsy_buf);	//extern에 채울것 담음
				}
			}

			for(i=0; i<strlen(all_buf); i++){
				if(all_buf[i] == '\n')
					all_buf[i] = 0;
			}			
			strcat(r_opt_java[r_java_count++], all_buf);	//자바코드 담음

			//전부아니면 그대로 내보냄
			if(!Match){
				strcat(result_list, imsy_buf);
			}
			else{	//부분적으로 바뀌지않은건 그대로 추가
				for(i=0; i<token_length; i++){
					strcat(result_list, token_buf[i]);
				}
			}

			for(i=0; i<token_length; i++){
				if(for_if_indent && strcmp(token_buf[i], "{") == 0)
					for_if_indent = 0;	//for, if문이 나왔었지만 바로 중괄호 출현시 무로돌림
			}

			//tab추가
			if(fd_count > 0){
				
				for(i=0; i<token_length; i++){	//{가 나오면 추가인덴트는 소멸함
					if(strcmp(token_buf[i], "{") == 0){
						if(before_add_indent){
							before_add_indent = 0;
							indent_count--;
						}
					}
				}
				
				char* tab_buf = "	";
				for(i=0; i<indent_count; i++){
					write(fd_list[fd_count-1], tab_buf, strlen(tab_buf));
					strcat(line_buf, tab_buf);
				}
				//해당내용 입력
				int rlen = strlen(result_list);
				write(fd_list[fd_count-1], result_list, rlen);
				strcat(line_buf, result_list);
				for(i=0; i<strlen(line_buf); i++){
					if(line_buf[i] == '\n')
						line_buf[i] = 0;
				}
				strcpy(r_opt_buf[r_opt_count++], line_buf);	//r옵션에 출력할 내용 담음
			}

			//다음행에 반영될 탭 카운트 개수 적용
			if(brace_check == 0){
				for(i=0; i<token_length; i++){
					if(strcmp(token_buf[i], "{") == 0){
						brace_indent[brace_count++] = indent_count;		//{일때의 indent_count를 기억함
						indent_count++;
					}
					else if(strcmp(token_buf[i], "}") == 0)
						indent_count--;
				}
			}
			else
				brace_check = 0;	//brace_check가 1이면 다시 0으로 돌려놓음
			
			if(before_add_indent == 0 && for_if_indent){	//if문이 처음 사용되었으면
				indent_count++;	//개행카운트를 늘림(if, for문에 중괄호가 없을수도있으므로)
				before_add_indent = 1;	//값을 더했다는것을 기억함
			}
			else if(before_add_indent){ //이전에 값을 증가시켰었다면
				indent_count--;
				before_add_indent = 0;
			}
			
			//r옵션시에만 작동함
			if(pid == 0 && o_r == 1){
				if(hang_java == 1){	//첫행일때만 before_where업데이트 
					before_where = where_count;
				}
				if(before_where != where_count && where_count > 2){	//보는 파일이 바뀌었을때 작동
					hang_c = 1;
					before_where = where_count;
					reset_token(r_opt_buf, r_opt_count);
					r_opt_count = 0;
					system("clear");
					printf("%s.c converting is finished!\n", where[where_count-2]);
					sleep(1);
				}

				system("clear");
				printf("%s Converting....\n", fname);
				printf("--------\n");
				printf("%s\n", fname);
				printf("--------\n");
				for(i=0; i<hang_java; i++){	//자바코드 한줄씩 출력
					printf("%d %s\n", i+1, r_opt_java[i]);
				}
				printf("--------\n");
				if(where_count == 1){
					printf("Can't find class yet\n");
				}
				else{
					printf("%s.c\n", where[where_count-1]);
					printf("--------\n");
					for(i=0; i<hang_c; i++){	//c코드 한줄씩 출력
						printf("%d %s\n", i+1, r_opt_buf[i]);
					}
					hang_c++;
				}

				hang_java++;
				sleep(1);
			}

			//2차처리까지 완료 - 버퍼초기화
			reset_buf(imsy_buf);
			reset_token(token_buf, token_length);
			reset_buf(line_buf);
			reset_buf(all_buf);
		}
		else{
			strncat(imsy_buf, &t, 1);
		}
	}
	
	if(pid == 0 && o_r == 1){	//마지막 파일변환 표시 후 지움
		system("clear");
		printf("%s.c converting is finished!\n", where[where_count-1]);
		sleep(1);
		system("clear");
	}

	//다 읽고나서 define과 header 처리
	for(i=0; i<fd_count; i++){
		int allsize = lseek(fd_list[i], 0, SEEK_END);
		char* tmp = (char*)malloc(allsize);
		char definebuf[BUFFER_SIZE] = {0};
		lseek(fd_list[i], 0, SEEK_SET);

		read(fd_list[i], tmp, allsize);	//임시보관

		lseek(fd_list[i], 0, SEEK_SET);

		if(IO_header_need[i+1]){	//헤더 추가
			write(fd_list[i], IO_header, strlen(IO_header));
		}

		if(exit_header_need[i+1]){	//헤더추가
			write(fd_list[i], exit_header, strlen(exit_header));
		}

		if(rw_header_need[i+1]){
			write(fd_list[i], rw_header, strlen(rw_header));
		}

		write(fd_list[i], "\n", strlen("\n")); //개행으로 정리

		if(i == fd_count - 1){ //마지막 fd, 즉 main일 경우
			char result_extern[BUFFER_SIZE] = {0};
			for(j=0; j<extern_count; j++){
				sprintf(result_extern, "extern %s;\n", extern_space[j]);
				write(fd_list[i], result_extern, strlen(result_extern));
			}
			
			if(extern_count > 0)
			write(fd_list[i], "\n", strlen("\n"));	//개행으로 정리

		}
	
		//이곳은 DEFINE
		char result_define[BUFFER_SIZE] = {0};
		for(j=0; j<define_count; j++){
			sprintf(definebuf, "#define %s %s\n", define_space[j], define_var[j]);
			strcat(result_define, definebuf);
		}
		write(fd_list[i], result_define, strlen(result_define));

		write(fd_list[i], tmp, strlen(tmp));	//이후 모든값 다시 붙여넣기
		free(tmp);
	}
	
	//이곳까지 도달시 변환은 성공적임
	for(i=1; i<where_count; i++){
		printf("%s.c convert Success!\n", where[i]);
	}

	//옵션영역
	if(o_c){	//c옵션 - 변환된 C 언어 프로그램 출력
		int hang = 1;
		char c;
		for(i=0; i<fd_count; i++){
			lseek(fd_list[i], 0, SEEK_SET);
			char printf_list[BUFFER_SIZE] = {0};
			while(read(fd_list[i], &c, 1) > 0){
				if(c == '\n'){
					printf("%d %s\n", hang++, printf_list);
					reset_buf(printf_list);
					continue;
				}
				strncat(printf_list, &c, 1);
			}
			hang = 1;
			printf("\n");
		}
	}

	if(o_j){	//j옵션 - 변환할 Java 언어 프로그램 코드 출력
		int hang = 1;
		char c;
		char printf_list[BUFFER_SIZE] = {0};

		lseek(fd, 0, SEEK_SET);
		while(read(fd, &c, 1) > 0){
			if(c == '\n'){
				printf("%d %s\n", hang++, printf_list);
				reset_buf(printf_list);
				continue;
			}
			strncat(printf_list, &c, 1);
		}
	}

	if(o_p){	//p옵션 - Java 언어 프로그램에서 사용된 함수들을 C에서 대응 되는 함수와 함께 출력
		int hang = 1;
		for(i=0; i<CON_FN; i++){
			if(p_opt_check[i] == 1)
				printf("%d %s", hang++, convert_functions[i]);
		}
	}

	if(o_f){	//f옵션 - Java 언어 프로그램 파일 및 C언어 프로그램 파일크기 출력
		int size = lseek(fd, 0, SEEK_END);
		printf("%s file size is %d bytes\n", fname, size);

		for(i=0; i<fd_count; i++){
			size = lseek(fd_list[i], 0, SEEK_END);
			printf("%s.c file size is %d bytes\n", where[i+1], size);
		}
	}

	if(o_l){	//l옵션 - Java언어 프로그램 파일 및 C언어 프로그램 파일의 라인 수 출력
		int hang = 0;
		char c;

		lseek(fd, 0, SEEK_SET);
		while(read(fd, &c, 1) > 0){
			if(c == '\n')
				hang++;
		}

		printf("%s line number is %d lines\n", fname, hang);
		hang = 0;

		for(i=0; i<fd_count; i++){
			lseek(fd_list[i], 0, SEEK_SET);
			char printf_list[BUFFER_SIZE] = {0};
			while(read(fd_list[i], &c, 1) > 0){
				if(c == '\n'){
					hang++;
				}
			}
			printf("%s.c line number is %d lines\n", where[i+1], hang);
			hang = 0;
		}
	}

	//makefile
	int mk_fd;
	char mk_name[MAX] = {0};
	char mk_buf[MAX] = {0};
	char mk_temp[MAX] = {0};

	//첫번째 줄
	sprintf(mk_name, "%s_Makefile", token_fname);
	if((mk_fd = open(mk_name, O_WRONLY | O_CREAT | O_TRUNC, 0777)) < 0){
		fprintf(stderr, "Can't make makefile\n");
		exit(1);
	}
	
	sprintf(mk_buf, "%s: ", token_fname);
	write(mk_fd, mk_buf, strlen(mk_buf));
	reset_buf(mk_buf);

	for(i=1; i<where_count; i++){
		sprintf(mk_temp, "%s.o ", where[i]);
		strcat(mk_buf, mk_temp);
	}
	write(mk_fd, mk_buf, strlen(mk_buf));
	write(mk_fd, "\n	", strlen("\n	"));

	sprintf(mk_temp, "gcc -o %s %s\n\n", token_fname, mk_buf);
	write(mk_fd, mk_temp, strlen(mk_temp));
	
	//2번째줄
	for(i=1; i<where_count; i++){
		sprintf(mk_buf, "%s.o: %s.c\n	gcc -c -o %s.o %s.c\n\n", where[i], where[i], where[i], where[i]);
		write(mk_fd, mk_buf, strlen(mk_buf));
	}
}

void reset_buf(char* imsy_buf)
{
	int len = strlen(imsy_buf);
	for(int i=0; i<len; i++)
		imsy_buf[i] = 0;
}

void reset_token(char(*token_list)[BUFFER_SIZE], int len)
{
	for(int i=0; i<len; i++){
		strcpy(token_list[i], "");
	}
}

void token_delete(char(*token_list)[BUFFER_SIZE], int len, char* delimiter)
{
	for(int i=0; i<len; i++){
		if(strcmp(token_list[i], delimiter) == 0){
			strcpy(token_list[i], "");
			if(token_list[i+1], "\n");
				strcpy(token_list[i+1], "");
			break;
		}
		strcpy(token_list[i], "");
		if(i < len - 1)
			if(token_list[i+1], "\n");
				strcpy(token_list[i+1], "");
	}
}
