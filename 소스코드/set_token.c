#include<stdio.h>
#include<string.h>

#define MAX 1005
#define BUFFER_SIZE 2048
#define TWO_OPERATOR_LENGTH 11
#define IS_CHARACTER (t == '_' || ('0' <= t && t <= '9') || ('A' <= t && t <= 'Z') || ('a' <= t && t <= 'z'))
#define IS_BRACE (t == '[' || t == '{' || t == '(' || t == ')' || t == '}' || t == ']')

extern char where[MAX][MAX];
extern void pattern_work(char* list, char token_buf[MAX][BUFFER_SIZE], int length);

void temp_reset(char* temp);

int set_token(char* list, char (*token_list)[BUFFER_SIZE])
{
	char two_operator_list[20][MAX] = {
  "==", ">=", "<=", "||", "&&", "->", "!=", "|=", "&=", "+=", "*="};

	char temp[BUFFER_SIZE] = {0};
	int token_count = 0;
	int length = strlen(list);
	int i;
	char t = 0;

	for(i=0; i<length; i++){
		t = list[i];
		if(t == '\t'){	//탭이 나오면 토큰으로 만들어버리고 스킵함
			if(strlen(temp) == 0)	//아무것도없으면 스킵함
				continue;
			strcpy(token_list[token_count++], temp);
			temp_reset(temp);
			continue;
		}
		else if(IS_BRACE){	//괄호류는 별개로 토큰화함
			if(strlen(temp) > 0){
				strcpy(token_list[token_count++], temp);
				temp_reset(temp);
			}

			strncat(temp, &t, 1);
			strcpy(token_list[token_count++], temp);
			temp_reset(temp);
			continue;
		}
		else if(IS_CHARACTER){	//일반문자는 버퍼에 담고 진행
			strncat(temp, &t, 1);
		}
		else if(t == ' '){
			if(strlen(temp) > 0){
				strcpy(token_list[token_count++], temp);
				temp_reset(temp);
			}
			strcpy(token_list[token_count++], " ");
		}
		else{	//연산자면 토큰으로 만들어버리고 연산자처리시작
			if(strlen(temp) > 0){	//토큰화할 데이터가있으면 토큰화하고 진행
				strcpy(token_list[token_count++], temp);
				temp_reset(temp);
			}
		
			strncat(temp, &t, 1);	//위에서 이전데이터가 토큰이되었고, 연산자를 임시버퍼에 저장

			if(i != length-1){	//다음 토큰 확인시작
				t = list[i+1];

				if(t == ' ' || t == '\t' || IS_CHARACTER){	//공백이나 문자가 다음문자였으면 토큰화 후 스킵
					strcpy(token_list[token_count++], temp);
					temp_reset(temp);
					continue;
				}
				else{	//연산자가 두번나타남
					strncat(temp, &t, 1);	//두 연산자를 붙여 비교를 시작함
					for(int j=0; j<TWO_OPERATOR_LENGTH; j++){
						if(strcmp(temp, two_operator_list[j]) == 0){
							strcpy(token_list[token_count++], temp);
							temp_reset(temp);
							i++;	//이중연산자였으므로 다음토큰은 검사치않음
							break;
						}
					}

					//그냥 별개의 연산자들이었다면 ex: ")
					int temp_end = strlen(temp);
					temp[temp_end-1] = 0;
					strcpy(token_list[token_count++], temp);
					temp_reset(temp);
				}
			}
			else{	//리스트의 끝이었다면
				strcpy(token_list[token_count++], temp);	//토큰으로 넣어줌
				temp_reset(temp);
			}
		}
	}
	if(strlen(temp) > 0)
		strcpy(token_list[token_count++], temp);	//남은 temp를 토큰으로

	return token_count;
}

void temp_reset(char* temp)
{
	int length = strlen(temp);
	for(int i=0; i<length; i++){
		temp[i] = '\0';
	}
}
