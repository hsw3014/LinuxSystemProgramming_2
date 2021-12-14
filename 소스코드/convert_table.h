#ifndef _TABLE_
#define _TABLE_

#define BUFFER_SIZE 2048
#define T_BUFFER_SIZE
#define MAX 305
#define TYPE_N 14
#define TYPE_E 8
#define CON_FN 6

void convert_start(int oj, int oc, int op, int of, int ol, int o_r);

int set_token(char* list, char(*token_list)[BUFFER_SIZE]);
void temp_reset(char* temp);
void pattern_work(char* list, char token_buf[MAX][BUFFER_SIZE], int length);

char line_buf[BUFFER_SIZE];		//엔터나오기전까지 담는버퍼

char token_buf[MAX][BUFFER_SIZE];	//토큰으로 나뉘어져 들어간 버퍼
char nospace_token_buf[MAX][BUFFER_SIZE];	//빈칸없는 토큰
char converted_buf[BUFFER_SIZE];	//바뀌어진 내용담는 버퍼

//strstr사용
char* print_keyword = "System.out.printf";	//printf로 치환
char* scanf_keyword = "nextInt();";	//scanf로 변경되고 별도저장된 변수로
char* Scann_keyword = "Scanner(System.in);";	//삭제되고 변수 별도저장
char* main_keyword = "static void main";
char* file_keyword = "new File(";	//char* 파일명 = " "로 변경
char* FileWriter_keyword = "new FileWriter(";	//open함수로 변경, strtok","
char* write_keyword = ".write(";	//C에맞게 변경해야함
char* flush_keyword = ".flush()";	//flush 앞앞토큰을 변수로
char* close_keyword = ".close()";	//close 앞앞토큰을 fd로

//token 사용
char* class_keyword = "class";	//.c파일로 전환, 이것 다음이 전환파일명 
char* new_keyword = "new";	//malloc해야함
char* pub_keyword = "public";	//삭제해버림(추가안함) 
char* imp_keyword = "import";	//삭제해버림(추가안함)
char* Scanner_keyword = "Scanner";	//Scanner가 type으로 쓰이면 날림
char* return_keyword = "return";	//main일때만 exit()으로 변경
char* static_keyword = "static";	//Define으로
char* for_keyword = "for";	//중괄호없으면 별도로 추가해야함
char* if_keyword = "if";
char* null_keyword = "null";	//NULL로
char* type_keyword[TYPE_N] = {
	"static", "boolean", "char", "byte", "short", "int", "long", "float", "double", "String", "final", "public", "private", "protected" };

char* except_type[TYPE_E] = {
	"char[]", "byte[]", "short[]", "int[]", "long[]", "float[]", "double[]", "String[]"};
char* matching_type[TYPE_E] = {
	"char*", "_Bool[]", "short*", "int*", "long*", "float*", "double*", "char*"};

char* new_type[TYPE_E] = {
	"new char[", "new byte[", "new short[", "new int[", "new long[", "new float[", "new double[", "new String["};
char* new_matching_type[TYPE_E] = {
	"(char*)malloc", "(_Bool*)malloc", "(short*)malloc", "(int*)malloc", "(long*)malloc", "(float*)malloc", "(double*)malloc", "(char*)malloc"};

char* convert_functions[CON_FN] = {
	"System.out.printf() -> printf()\n",
	"nextInt() -> scanf()\n",
	"FileWriter() -> fopen()\n",
	"write() -> fwrite()\n",
	"flush() -> fflush()\n",
	"close() -> fclose()\n",
};
#endif
