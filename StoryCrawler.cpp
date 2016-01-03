#include<iostream>
using namespace std;
#include"KENNYHTTPSOCKET.H"
#include"KENNYFILE.H"
#include"json/json.h"
#pragma comment(lib,"lib_json.lib")
FILE*ftmp;
char idarr[10010][20];
int idcnt;
int main(int argc,char *argv[])
{
#ifdef _DEBUG
	argc=2;
	//argv[1]="WU_OK1CZYtFTwIDKSFzk6F_3Udgefh1hW9R6Xz9qinYYgzC6PEj_SCmsZRUQJAS8zjLAcawQQHYAAAFSBqZE-Q";
#endif
	KENNYHTTPSOCKET req("POST"),req2,req3;
	int i,j,k;
	char header[1024],url[1024],content[1024],command[1024],file[1024],extension[32],progress[32];
	char code[1024],token[1024],stories[65536],story[16384];
	Json::Value json;
	Json::Reader reader;
	if(argc!=2)return 0;
	strcpy(code,argv[1]);
	sprintf(content,"grant_type=authorization_code&client_id=ff54d7552fee1cd457df311f85e5fa03&redirect_uri=http://kennysoft.github.io/StoryCrawler/web/oauth.html&code=%s",code);
	req.setUrl("https://kauth.kakao.com/oauth/token");
	req.setContent(content);
	req.responseBodyToFile("token.txt");
	KENNYFILE fp("token.txt");
	fp.findStringAndPush("\"access_token\":\"");
	fp.getString(token,"\"");
	sprintf(header,"Authorization: Bearer %s",token);
	req2.setHeader(header);
	while(1)
	{
		if(idcnt)sprintf(url,"https://kapi.kakao.com/v1/api/story/mystories?last_id=%s",idarr[idcnt-1]);
		else sprintf(url,"https://kapi.kakao.com/v1/api/story/mystories");
		req2.setUrl(url);
		req2.responseBodyToFile("stories.txt");
		ftmp=fopen("stories.txt","r");
		fgets(stories,sizeof(stories),ftmp);
		fclose(ftmp);
		reader.parse(stories,json);
		if(json.size()==0)break;
		for(i=0;i<json.size();++i)strcpy(idarr[idcnt++],json[i]["id"].asString().c_str());
		cout<<"Collecting... "<<idcnt<<" stories collected."<<endl;
#ifdef _DEBUG
		break;
#endif
	}
	for(i=idcnt-1;i>=0;--i)
	{
		cout<<idcnt-i<<" "<<idarr[i]<<" ";
		sprintf(url,"https://kapi.kakao.com/v1/api/story/mystory?id=%s",idarr[i]);
		req2.setUrl(url);
		req2.responseBodyToFile("story.txt");
		sprintf(command,"md \"%d %s\" 2>nul",idcnt-i, idarr[i]);
		system(command);
		ftmp=fopen("story.txt","r");
		fgets(story,sizeof(story),ftmp);
		fclose(ftmp);
		reader.parse(story,json);
		for(j=0;j<json["media"].size();++j)
		{
			sprintf(progress,"[%d/%d]",j,json["media"].size());
			cout<<progress;
			strcpy(file,json["media"][j]["original"].asString().c_str());
			for(k=strlen(file)-1;;--k)if(file[k]=='.')break;
			memset(extension,0,sizeof(extension));
			for(;file[k]!='?'&&file[k]!='\0';++k)extension[strlen(extension)]=file[k];
			sprintf(file,"img%d%s",j,extension);
			req3.setUrl(json["media"][j]["original"].asString().c_str());
			req3.responseBodyToFile(file);
			sprintf(command,"move %s \"%d %s\" >nul",file,idcnt-i,idarr[i]);
			system(command);
			for(k=0;k<strlen(progress);++k)cout<<"\b";
		}
		// TODO: story.txt parse and construct html
		// TODO: if NOT_SUPPORTED(share) get webpage and parse from there
		// TODO: information.txt - include storycrawler version, collected time
		sprintf(command,"move story.txt \"%d %s\" >nul",idcnt-i,idarr[i]);
		system(command);
		cout<<"["<<json["media"].size()<<"/"<<json["media"].size()<<"]"<<endl;
	}
	system("del login.txt 2>nul");
	system("del code.txt 2>nul");
	system("del token.txt 2>nul");
	system("del stories.txt 2>nul");
	system("del story.txt 2>nul");
	return 0;
}