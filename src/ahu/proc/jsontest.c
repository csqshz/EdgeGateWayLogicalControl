#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <cjson/cJSON_Utils.h>

#define file_json "./read.json"
#define file_wr   "./write.json"


int main(int argc, const char *argv[])
{
	char *temp = NULL;
	char *temp_unfmt = NULL;

	char *jsonStr = "{\"semantic\":{\"slots\":{\"name\":\"张三\"}}, \"rc\":0, \"operation\":\"CALL\", \"service\":\"telephone\", \"text\":\"打电话给张三\", \"num\":1356707}";

	printf("%s \n", jsonStr);

	cJSON * root = NULL;
	// convert string to cJson* object
	root = cJSON_Parse(jsonStr);
	if (!root) {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
		return -1;
    }

	// convert cJsong *object to string with format
	temp = cJSON_Print(root);
	printf("%s\n\n", temp);
	// convert cJsong *object to string with unformat
	temp_unfmt = cJSON_PrintUnformatted(root);
	printf("%s\n\n", temp_unfmt);

	int wrn;
	int fdwr;

	fdwr = open(file_wr, O_CREAT|O_RDWR|O_APPEND, 0777);
	if(fwrite < 0) {
		perror("Fail to open file_json");
		return -1;
	}

	wrn = write(fdwr, temp, strlen(temp));
	

	cJSON_Delete(root);
	free(temp);
	free(temp_unfmt);

	int fd;
	int rdn;
	char buf[2048];


	cJSON * root2 = NULL;
	cJSON * ObjGet = NULL;

	fd = open(file_json, O_RDWR, 0777);
	if(fd < 0) {
		perror("Fail to open file_json");
		return -1;
	}

	rdn = read(fd, buf, 2048);

	printf("buf[%d]=\n\n%s \n", rdn, buf);

	root2 = cJSON_Parse(buf);
	ObjGet = cJSON_GetObjectItem(root2, "RetTemp");
	printf("%s \n", cJSON_Print(ObjGet));

	ObjGet = cJSON_GetObjectItem(root2, "RetHmdt");
	printf("%s \n\n", cJSON_Print(ObjGet));

	ObjGet = cJSON_GetObjectItem(root2, "BlTemp");
	printf("%s \n\n", cJSON_Print(ObjGet));

	ObjGet = cJSON_GetObjectItem(root2, "BlHmdt");
	printf("%s \n\n", cJSON_Print(ObjGet));
/*
	temp = cJSON_Print(root2);
	printf("%s\n\n", temp);

	wrn = write(fdwr, temp, strlen(temp));
*/
	cJSON_Delete(root2);
	free(temp);

	close(fd);
	close(fdwr);

	return 0;

}
