#ifndef __KENNYHTTPSOCKET_H__
#define __KENNYHTTPSOCKET_H__
#define VERSION_KENNYHTTPSOCKET "3.5.1"
#define VERSION_OPENSSL "openssl-1.0.2h"
#pragma warning(disable:4786)
#include<fstream>
using namespace std;
#include<map>
#include<string>
#include<openssl/ssl.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"ssleay32.lib")
class KENNYHTTPSOCKET
{
public:
	KENNYHTTPSOCKET(const char *Method="GET",const char *Host="",const char *Path="",const char *Header="User-Agent: KENNYHTTPSOCKET/"VERSION_KENNYHTTPSOCKET,const char *Content="",unsigned int ContentLength=0)
	{
		setMethod(Method);
		setHost(Host);
		setPath(Path);
		header=NULL;
		setHeader(Header);
		content=NULL;
		setContent(Content,ContentLength);
		contentType[0]=0;
		memset(table,0,sizeof(table));
		table[(128+'\r')+256*0]=1;
		table[(128+'\n')+256*1]=2;
		table[(128+'\r')+256*2]=3;
		table[(128+'\n')+256*3]=4;
	}
	~KENNYHTTPSOCKET()
	{
		if(header)free(header);
		if(content)free(content);
	}
	void setMethod(const char *Method,unsigned int Length=0)
	{
		if(!Length)Length=strlen(Method);
		strncpy(method,Method,Length);
		method[Length]=0;
	}
	void setUrl(const char *Url)
	{
		if(!strncmp(Url,"https://",8))
		{
			if(!strstr(Url+8,"/"))
			{
				setHost(Url);
				setPath("/");
			}
			else
			{
				setHost(Url,strstr(Url+8,"/")-Url);
				setPath(strstr(Url+8,"/"));
			}
		}
		else if(!strncmp(Url,"http://",7))
		{
			if(!strstr(Url+7,"/"))
			{
				setHost(Url);
				setPath("/");
			}
			else
			{
				setHost(Url,strstr(Url+7,"/")-Url);
				setPath(strstr(Url+7,"/"));
			}
		}
		else
		{
			if(!strstr(Url,"/"))
			{
				setHost(Url);
				setPath("/");
			}
			else
			{
				setHost(Url,strstr(Url,"/")-Url);
				setPath(strstr(Url,"/"));
			}
		}
	}
	void setHost(const char *Host,unsigned int Length=0)
	{
		if(!Length)Length=strlen(Host);
		strncpy(host,Host,Length);
		host[Length]=0;
		if(!strncmp(host,"https://",8))
		{
			isHttps=true;
			setPort(443);
			memmove(host,host+8,strlen(host)-7);
		}
		else
		{
			isHttps=false;
			setPort(80);
			if(!strncmp(host,"http://",7))memmove(host,host+7,strlen(host)-6);
		}
		if(strstr(host,":"))
		{
			setPort(atoi(strstr(host,":")+1));
			*strstr(host,":")=0;
		}
	}
	void setPort(unsigned int Port)
	{
		port=Port;
	}
	void setPath(const char *Path,unsigned int PathLength=0)
	{
		if(!PathLength)PathLength=strlen(Path);
		if(Path[0]=='/')
		{
			strncpy(path,Path,PathLength);
			path[PathLength]=0;
		}
		else
		{
			path[0]='/';
			strncpy(path+1,Path,PathLength);
			path[PathLength+1]=0;
		}
	}
	void setHeader(const char *Header,unsigned int HeaderLength=0)
	{
		int i,j;
		char cn[1024],cv[1024];
		if(!HeaderLength)headerLength=strlen(Header);
		else headerLength=HeaderLength;
		if(header)free(header);
		header=(char*)malloc(headerLength+5);
		strncpy(header,Header,headerLength);
		header[headerLength]=0;
		while(header[headerLength-1]=='\r'||header[headerLength-1]=='\n')header[(headerLength--)-1]=0;
		strcat(header,"\r\n");
		for(i=0;i<headerLength;)
		{
			if(!strncmp(header+i,"Connection",10)||!strncmp(header+i,"Content-Length",14)||!strncmp(header+i,"Cookie",6)||!strncmp(header+i,"Host",4))
			{
				if(!strncmp(header+i,"Cookie",6))
				{
					for(j=i+8;j<strstr(header+i,"\r\n")-header;)
					{
						sscanf(header+j,"%[^=]s",cn);
						j=strstr(header+j,"=")+1-header;
						if(strstr(header+j,";"))
						{
							sscanf(header+j,"%[^;]s",cv);
							j=strstr(header+j,";")+2-header;
						}
						else
						{
							sscanf(header+j,"%[^\r]s",cv);
							j=strstr(header+i,"\r")-header;
						}
						setCookie(cn,cv);
					}
				}
				memmove(header+i,strstr(header+i,"\r\n")+2,headerLength-(strstr(header+i,"\r\n")-header)+1);
				headerLength=strlen(header);
			}
			else i=strstr(header+i,"\r\n")+2-header;
		}
		while(header[strlen(header)-1]=='\r'||header[strlen(header)-1]=='\n')header[strlen(header)-1]=0;
		strcat(header,"\r\n\r\n");
		isRawHeader=false;
	}
	void setRawHeader(const char *Header,unsigned int HeaderLength=0)
	{
		if(!HeaderLength)headerLength=strlen(Header);
		else headerLength=HeaderLength;
		if(header)free(header);
		header=(char*)malloc(headerLength+1);
		strncpy(header,Header,headerLength);
		header[headerLength]=0;
		isRawHeader=true;
	}
	void setContent(const char *Content,unsigned int ContentLength=0)
	{
		if(!ContentLength)contentLength=strlen(Content);
		else contentLength=ContentLength;
		if(content)free(content);
		content=(char*)malloc(contentLength+1);
		strncpy(content,Content,contentLength);
		content[contentLength]=0;
	}
	void setContentType(const char *ContentType,unsigned int ContentTypeLength=0)
	{
		if(!ContentTypeLength)ContentTypeLength=strlen(ContentType);
		strncpy(contentType,ContentType,ContentTypeLength);
		contentType[ContentTypeLength]=0;
	}
	void setCookie(const char *CookieName,const char *CookieValue)
	{
		cookie[(string)CookieName]=(string)CookieValue;
	}
	void deleteCookie(const char *CookieName)
	{
		cookie.erase((string)CookieName);
	}
	void createMessage(char* &message)
	{
		char tmp[16384];
		map<string,string>::iterator it;
		message=(char*)malloc(16384+contentLength);
		message[0]=0;
		sprintf(message,"%s %s HTTP/1.1\r\n",method,path);
		if(!isRawHeader)
		{
			sprintf(tmp,"Host: %s\r\n",host);
			strcat(message,tmp);
			sprintf(tmp,"Connection: close\r\n");
			strcat(message,tmp);
			if(contentLength)
			{
				sprintf(tmp,"Content-Length: %d\r\n",contentLength);
				strcat(message,tmp);
				if(strlen(contentType))sprintf(tmp,"Content-Type: %s\r\n",contentType);
				else sprintf(tmp,"Content-Type: application/x-www-form-urlencoded\r\n");
				strcat(message,tmp);
			}
			if(cookie.size())
			{
				sprintf(tmp,"Cookie: ");
				strcat(message,tmp);
				for(it=cookie.begin();it!=cookie.end();++it)
				{
					sprintf(tmp,"%s=%s",it->first.c_str(),it->second.c_str());
					strcat(message,tmp);
					strcat(message,"; ");
				}
				message[strlen(message)-1]=0;
				message[strlen(message)-1]=0;
				strcat(message,"\r\n");
			}
			strcat(message,header);
			messageLength=strlen(message);
			strncat(message,content,contentLength);
			messageLength=messageLength+contentLength;
			message[messageLength]=0;
		}
		else
		{
			strcat(message,header);
			messageLength=strlen(message);
		}
	}
	int getResponseCode()
	{
		int ret;
		char *response=getResponseHeader();
		if(response==NULL)return 0;
		sscanf(response,"HTTP/1.1 %d",&ret);
		return ret;
	}
	char* getResponseHeader()
	{
		int i,state=0,recvcnt,recvtot=0;
		char tmp[1024],*message=NULL,*response=NULL;
		WSADATA Data;
		SOCKADDR_IN recvSockAddr={0};
		SOCKET recvSocket;
		SSL *ssl;
		if(WSAStartup(MAKEWORD(2,2),&Data))return NULL;
		recvSockAddr.sin_port=htons(port);
		recvSockAddr.sin_family=AF_INET;
		recvSockAddr.sin_addr.s_addr=inet_addr(inet_ntoa(*(in_addr*)*(gethostbyname(host))->h_addr_list));
		recvSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(recvSocket==INVALID_SOCKET)goto end;
		if(connect(recvSocket,(SOCKADDR*)&recvSockAddr,sizeof(SOCKADDR_IN))==SOCKET_ERROR)goto end;
		createMessage(message);
		if(isHttps)
		{
			SSLeay_add_ssl_algorithms();
			SSL_load_error_strings();
			SSL_library_init();
			ssl=SSL_new(SSL_CTX_new(SSLv23_client_method()));
			SSL_set_fd(ssl,recvSocket);
			SSL_connect(ssl);
			if(SSL_write(ssl,message,messageLength)<0)goto end;
		}
		else if(send(recvSocket,message,messageLength,0)==SOCKET_ERROR)goto end;
		response=(char*)malloc(16384);
		response[0]=0;
		while(1)
		{
			memset(buffer,0,sizeof(buffer));
			if(isHttps)recvcnt=SSL_read(ssl,buffer,sizeof(buffer));
			else recvcnt=recv(recvSocket,buffer,sizeof(buffer),0);
			if(recvcnt==SOCKET_ERROR)
			{
				free(response);
				response=NULL;
				break;
			}
			else if(recvcnt==0)break;
			else
			{
				for(i=0;i<recvcnt;++i)
				{
					tmp[0]=buffer[i];
					tmp[1]=0;
					strcat(response,tmp);
					state=table[(128+buffer[i])+256*state];
					if(state==4)goto end;
				}
			}
		}
end:
		if(message)free(message);
		if(isHttps&&ssl)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
		if(recvSocket!=INVALID_SOCKET)closesocket(recvSocket);
		WSACleanup();
		return response;
	}
	int getResponseBodyToFile(const char *FileName,bool allowMove=true,unsigned int remainMove=10)
	{
		int i,state=0,recvcnt,chunksize=0,ret=-1,length=-1;
		bool isHeaderStarted=false,isContentStarted=false,isChunked=false,isMoved=false;
		char tmp[1024],*message=NULL,*response=NULL,location[1024];
		FILE *fp=NULL;
		WSADATA Data;
		SOCKADDR_IN recvSockAddr={0};
		SOCKET recvSocket=INVALID_SOCKET;
		SSL *ssl;
		fp=fopen(FileName,"wb");
		if(!fp)return -1;
		if(WSAStartup(MAKEWORD(2,2),&Data))return -2;
		recvSockAddr.sin_port=htons(port);
		recvSockAddr.sin_family=AF_INET;
		recvSockAddr.sin_addr.s_addr=inet_addr(inet_ntoa(*(in_addr*)*(gethostbyname(host))->h_addr_list));
		recvSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(recvSocket==INVALID_SOCKET)goto end;
		if(connect(recvSocket,(SOCKADDR*)&recvSockAddr,sizeof(SOCKADDR_IN))==SOCKET_ERROR)goto end;
		createMessage(message);
		if(isHttps)
		{
			SSLeay_add_ssl_algorithms();
			SSL_load_error_strings();
			SSL_library_init();
			ssl=SSL_new(SSL_CTX_new(SSLv23_client_method()));
			SSL_set_fd(ssl,recvSocket);
			SSL_connect(ssl);
			if(SSL_write(ssl,message,messageLength)<0)goto end;
		}
		else if(send(recvSocket,message,messageLength,0)==SOCKET_ERROR)goto end;
		response=(char*)malloc(16384);
		response[0]=0;
		while(length)
		{
			memset(buffer,0,sizeof(buffer));
			if(isHttps)recvcnt=SSL_read(ssl,buffer,sizeof(buffer));
			else recvcnt=recv(recvSocket,buffer,sizeof(buffer),0);
			if(recvcnt==SOCKET_ERROR)
			{
				ret=-3;
				goto end;
			}
			else if(recvcnt==0)break;
			else
			{
				if(!isContentStarted)
				{
					for(i=0;i<recvcnt;++i)
					{
						tmp[0]=buffer[i];
						tmp[1]=0;
						strcat(response,tmp);
						state=table[(128+buffer[i])+256*state];
						if(state==4)
						{
							sscanf(response,"HTTP/1.1 %d",&ret);
							if(ret/100==3&&allowMove)
							{
								sscanf(strstr(response,"Location: ")+10,"%s",location);
								isMoved=true;
								goto end;
							}
							if(strstr(response,"Content-Length: "))sscanf(strstr(response,"Content-Length: ")+16,"%d",&length);
							if(strstr(response,"Transfer-Encoding: chunked"))isChunked=true;
							i++;
							if(isChunked)
							{
								while(i<recvcnt)
								{
									if(chunksize==0)
									{
										sscanf(buffer+i,"%x",&chunksize);
										if(chunksize==0)break;
										chunksize=chunksize+2;
										for(;;++i)if(buffer[i]=='\r'&&buffer[i+1]=='\n')break;
										i=i+2;
									}
									for(;i<recvcnt;++i)
									{
										if(chunksize==0)break;
										if(chunksize>2)
										{
											fwrite(buffer+i,1,1,fp);
											length--;
										}
										chunksize--;
									}
								}
							}
							else
							{
								fwrite(buffer+i,1,recvcnt-i,fp);
								length=length-recvcnt+i;
							}
							isContentStarted=true;
							break;
						}
					}
				}
				else
				{
					if(isChunked)
					{
						i=0;
						while(i<recvcnt)
						{
							if(chunksize==0)
							{
								sscanf(buffer+i,"%x",&chunksize);
								if(chunksize==0)break;
								chunksize=chunksize+2;
								for(;;++i)if(buffer[i]=='\r'&&buffer[i+1]=='\n')break;
								i=i+2;
							}
							for(;i<recvcnt;++i)
							{
								if(chunksize==0)break;
								if(chunksize>2)
								{
									fwrite(buffer+i,1,1,fp);
									length--;
								}
								chunksize--;
							}
						}
					}
					else
					{
						fwrite(buffer,1,recvcnt,fp);
						length=length-recvcnt;
					}
				}
			}
		}
end:
		if(response)free(response);
		if(message)free(message);
		if(fp)fclose(fp);
		if(isHttps&&ssl)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
		if(recvSocket!=INVALID_SOCKET)closesocket(recvSocket);
		WSACleanup();
		if(isMoved&&remainMove)
		{
			KENNYHTTPSOCKET *req=(KENNYHTTPSOCKET*)malloc(sizeof(KENNYHTTPSOCKET));
			memcpy(req,this,sizeof(KENNYHTTPSOCKET));
			req->setUrl(location);
			ret=req->getResponseBodyToFile(FileName,true,remainMove-1);
			free(req);
		}
		return ret;
	}
private:
	char method[16],host[1024],path[1024],contentType[1024],buffer[16384];
	char *header,*content;
	map<string,string> cookie;
	int port,table[1024],headerLength,contentLength,messageLength;
	bool isHttps,isRawHeader;
};
#endif