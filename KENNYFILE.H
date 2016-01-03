#ifndef __KENNYFILE_H__
#define __KENNYFILE_H__
#define VERSION_KENNYFILE "1.5.1"
#pragma warning(disable:4786)
#include<fstream>
using namespace std;
#include<queue>
#include<string.h>
class KENNYFILE
{
public:
	KENNYFILE(const char *fileName=NULL,int bufferSize=1024)
	{
		fin=NULL;
		size=bufferSize;
		setFile(fileName);
	}
	~KENNYFILE()
	{
		fclose(fin);
	}
	void setFile(const char *fileName=NULL)
	{
		int i;
		char tmp;
		if(fin)
		{
			fclose(fin);
			while(!data.empty())data.pop_front();
		}
		if(fileName==NULL||fileName[0]==0)return;
		fin=fopen(fileName,"r");
		pos=0;
		for(i=1;i<=size;++i)
		{
			tmp=getc(fin);
			if(feof(fin))break;
			data.push_back(tmp);
		}
	}
	void reset()
	{
		int i;
		char tmp;
		while(!data.empty())data.pop_front();
		rewind(fin);
		pos=0;
		for(i=1;i<=size;++i)
		{
			tmp=getc(fin);
			if(feof(fin))break;
			data.push_back(tmp);
		}
	}
	bool push(unsigned int n=1)
	{
		int i;
		char tmp;
		if(!fin)return false;
		for(i=1;i<=n;++i)
		{
			if(data.empty())return false;
			data.pop_front();
			pos++;
			tmp=getc(fin);
			if(feof(fin))continue;
			data.push_back(tmp);
		}
		return true;
	}
	int getPosition()
	{
		return pos;
	}
	void setPosition(int newpos)
	{
		if(newpos>pos)push(newpos-pos);
		else
		{
			reset();
			push(newpos);
		}
	}
	int getCount(const char *str,const char *sta=NULL,const char *end=NULL)
	{
		int posbak=pos,ret=0;
		if(sta)
		{
			if(!findStringAndPush(sta))
			{
				reset();
				if(!findStringAndPush(sta))return -1;
			}
		}
		while(findStringAndPush(str,end))ret++;
		setPosition(posbak);
		return ret;
	}
	char* getString(char *str,unsigned int n)
	{
		int i;
		if(!fin||n>data.size()||n<0)return NULL;
		for(i=0;i<n;++i)str[i]=data[i];
		str[n]=0;
		return str;
	}
	char* getString(char *str,char *until)
	{
		int i;
		if(!fin)return NULL;
		for(i=0;i<data.size();++i)
		{
			if(!strcmp(until,i))
			{
				str[i]=0;
				break;
			}
			else str[i]=data[i];
		}
		return str;
	}
	bool findString(const char *str,const char *until=NULL)
	{
		if(!fin)return false;
		for(;!data.empty();push())
		{
			if(!strcmp(str))return true;
			if(until&&!strcmp(until))return false;
		}
		return false;
	}
	bool findStringAndPush(const char *str,const char *until=NULL)
	{
		bool ret=findString(str,until);
		if(ret)push(strlen(str));
		return ret;
	}
	int strcmp(const char *str,unsigned int offset=0)
	{
		int l=strlen(str),ret;
		char *org=(char*)malloc(size+1);
		if(l>data.size())return -1;
		getString(org,offset+strlen(str));
		ret=::strcmp(org+offset,str);
		free(org);
		return ret;
	}
private:
	FILE *fin;
	deque<char> data;
	int pos,size;
};
#endif