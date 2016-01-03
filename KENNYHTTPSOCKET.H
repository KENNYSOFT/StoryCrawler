#pragma warning(disable:4786)
#include<fstream>
using namespace std;
#include<cstring>
#include<vector>
#include<winsock.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wsock32.lib")
// openssl-1.0.2d
#include<openssl/rsa.h>
#include<openssl/crypto.h>
#include<openssl/x509.h>
#include<openssl/pem.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ssleay32.lib")
class KENNYHTTPSOCKET
{
public:
	KENNYHTTPSOCKET(const char *Method="GET",const char *Host="",const char *Path="",const char *Header="User-Agent: KENNYHTTPSOCKET/2.2.1",const char *Content="",int ContentLength=0)
	{
		setMethod(Method);
		setHost(Host);
		setPath(Path);
		header=NULL;
		setHeader(Header);
		content=NULL;
		setContent(Content,ContentLength);
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
	void setMethod(const char *Method,int Length=0)
	{
		if(!Length)Length=strlen(Method);
		strncpy(method,Method,Length);
		method[Length]=0;
	}
	void setUrl(const char *Url)
	{
		if(!strncmp(Url,"http://",7))
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
	}
	void setHost(const char *Host,int Length=0)
	{
		if(!Length)Length=strlen(Host);
		strncpy(host,Host,Length);
		host[Length]=0;
		if(!strncmp(host,"http://",7))
		{
			isHttps=false;
			memmove(host,host+7,strlen(host)-6);
		}
		if(!strncmp(host,"https://",8))
		{
			isHttps=true;
			memmove(host,host+8,strlen(host)-7);
		}
	}
	void setPath(const char *Path,int Length=0)
	{
		if(!Length)Length=strlen(Path);
		strncpy(path,Path,Length);
		path[Length]=0;
	}
	void setHeader(const char *Header,int HeaderLength=0)
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
	}
	void setContent(const char *Content,int ContentLength=0)
	{
		if(!ContentLength)contentLength=strlen(Content);
		else contentLength=ContentLength;
		if(content)free(content);
		content=(char*)malloc(contentLength+1);
		strncpy(content,Content,contentLength);
		content[contentLength]=0;
	}
	void setCookie(const char *CookieName,const char *CookieValue)
	{
		unsigned int i;
		bool isComplete=false;
		for(i=0;i<cookieName.size();++i)
		{
			if(!strcmp(cookieName[i],CookieName))
			{
				free(cookieValue[i]);
				cookieValue[i]=(char*)malloc(strlen(CookieValue)+1);
				strcpy(cookieValue[i],CookieValue);
				isComplete=true;
			}
		}
		if(!isComplete)
		{
			cookieName.push_back((char*)malloc(strlen(CookieName)+1));
			strcpy(cookieName[cookieName.size()-1],CookieName);
			cookieValue.push_back((char*)malloc(strlen(CookieValue)+1));
			strcpy(cookieValue[cookieValue.size()-1],CookieValue);
		}
	}
	void deleteCookie(const char *CookieName)
	{
		// TODO
	}
	int responseHeaderToFile(const char *FileName)
	{
		int i,state=0,recvcnt,chunksize=0,ret=0;
		bool isHeaderStarted=false,isChunked=false;
		char tmp[16384],*message=NULL;
		FILE *fp=NULL;
		WSADATA Data;
		SOCKADDR_IN recvSockAddr;
		SOCKET recvSocket;
		SSL *conn;
		if(WSAStartup(MAKEWORD(2,2),&Data))return 0;
		memset(&recvSockAddr,0,sizeof(recvSockAddr));
		if(isHttps)recvSockAddr.sin_port=htons(443);
		else recvSockAddr.sin_port=htons(80);
		recvSockAddr.sin_family=AF_INET;
		recvSockAddr.sin_addr.s_addr=inet_addr(inet_ntoa(*(in_addr*)*(gethostbyname(host))->h_addr_list));
		recvSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(recvSocket==INVALID_SOCKET)
		{
			WSACleanup();
			return 0;
		}
		if(connect(recvSocket,(SOCKADDR*)&recvSockAddr,sizeof(SOCKADDR_IN))==SOCKET_ERROR)
		{
			closesocket(recvSocket);
			WSACleanup();
			return 0;
		}
		message=(char*)malloc(16384+contentLength);
		message[0]=0;
		sprintf(tmp,"%s %s HTTP/1.1\r\n",method,path);
		strcat(message,tmp);
		sprintf(tmp,"Host: %s\r\n",host);
		strcat(message,tmp);
		sprintf(tmp,"Connection: close\r\n");
		strcat(message,tmp);
		if(contentLength)
		{
			sprintf(tmp,"Content-Length: %d\r\n",contentLength);
			strcat(message,tmp);
			sprintf(tmp,"Content-Type: application/x-www-form-urlencoded\r\n");
			strcat(message,tmp);
		}
		strcat(tmp,"Cookie: ");
		strcat(message,tmp);
		for(i=0;i<cookieName.size();++i)
		{
			sprintf(tmp,"%s=%s",cookieName[i],cookieValue[i]);
			strcat(message,tmp);
			if(i<cookieName.size()-1)strcat(message,"; ");
		}
		strcat(message,"\r\n");
		strcat(message,header);
		messageLength=strlen(message);
		strncat(message,content,contentLength);
		messageLength=messageLength+contentLength;
		message[messageLength]=0;
		if(isHttps)
		{
			SSLeay_add_ssl_algorithms();
			SSL_load_error_strings();
			SSL_library_init();
			conn=SSL_new(SSL_CTX_new(SSLv23_client_method()));
			SSL_set_fd(conn,recvSocket);
			SSL_connect(conn);
			if(SSL_write(conn,message,messageLength)<0)goto end;
		}
		else if(send(recvSocket,message,messageLength,0)==SOCKET_ERROR)goto end;
		fp=fopen(FileName,"wb");
		while(1)
		{
			memset(buffer,0,sizeof(buffer));
			if(isHttps)recvcnt=SSL_read(conn,buffer,sizeof(buffer));
			else recvcnt=recv(recvSocket,buffer,sizeof(buffer),0);
			if(!isHeaderStarted)
			{
				sscanf(buffer,"HTTP/1.1 %d",&ret);
				isHeaderStarted=true;
			}
			if(strstr(buffer,"Transfer-Encoding: chunked"))isChunked=true;
			if(recvcnt==SOCKET_ERROR)
			{
				ret=0;
				break;
			}
			else if(recvcnt==0)break;
			else
			{
				for(i=0;i<recvcnt;++i)
				{
					state=table[(128+buffer[i])+256*state];
					fwrite(buffer+i,1,1,fp);
					if(state==4)break;
				}
			}
		}
end:
		if(message)free(message);
		if(fp)fclose(fp);
		if(isHttps)
		{
			SSL_shutdown(conn);
			SSL_free(conn);
		}
		closesocket(recvSocket);
		WSACleanup();
		return ret;
	}
	int responseBodyToFile(const char *FileName,bool allowMove=true)
	{
		int i,state=0,recvcnt,chunksize=0,ret=0;
		bool isHeaderStarted=false,isContentStarted=false,isChunked=false,isMoved=false;
		char tmp[16384],*message=NULL,location[1024];
		FILE *fp=NULL;
		WSADATA Data;
		SOCKADDR_IN recvSockAddr;
		SOCKET recvSocket;
		SSL *conn;
		if(WSAStartup(MAKEWORD(2,2),&Data))return 0;
		memset(&recvSockAddr,0,sizeof(recvSockAddr));
		if(isHttps)recvSockAddr.sin_port=htons(443);
		else recvSockAddr.sin_port=htons(80);
		recvSockAddr.sin_family=AF_INET;
		recvSockAddr.sin_addr.s_addr=inet_addr(inet_ntoa(*(in_addr*)*(gethostbyname(host))->h_addr_list));
		recvSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(recvSocket==INVALID_SOCKET)
		{
			WSACleanup();
			return 0;
		}
		if(connect(recvSocket,(SOCKADDR*)&recvSockAddr,sizeof(SOCKADDR_IN))==SOCKET_ERROR)
		{
			closesocket(recvSocket);
			WSACleanup();
			return 0;
		}
		message=(char*)malloc(16384+contentLength);
		message[0]=0;
		sprintf(tmp,"%s %s HTTP/1.1\r\n",method,path);
		strcat(message,tmp);
		sprintf(tmp,"Host: %s\r\n",host);
		strcat(message,tmp);
		sprintf(tmp,"Connection: close\r\n");
		strcat(message,tmp);
		if(contentLength)
		{
			sprintf(tmp,"Content-Length: %d\r\n",contentLength);
			strcat(message,tmp);
			sprintf(tmp,"Content-Type: application/x-www-form-urlencoded\r\n");
			strcat(message,tmp);
		}
		strcat(message,"Cookie: ");
		for(i=0;i<cookieName.size();++i)
		{
			sprintf(tmp,"%s=%s",cookieName[i],cookieValue[i]);
			strcat(message,tmp);
			if(i<cookieName.size()-1)strcat(message,"; ");
		}
		strcat(message,"\r\n");
		strcat(message,header);
		messageLength=strlen(message);
		strncat(message,content,contentLength);
		messageLength=messageLength+contentLength;
		message[messageLength]=0;
		if(isHttps)
		{
			SSLeay_add_ssl_algorithms();
			SSL_load_error_strings();
			SSL_library_init();
			conn=SSL_new(SSL_CTX_new(SSLv23_client_method()));
			SSL_set_fd(conn,recvSocket);
			SSL_connect(conn);
			if(SSL_write(conn,message,messageLength)<0)goto end;
		}
		else if(send(recvSocket,message,messageLength,0)==SOCKET_ERROR)goto end;
		fp=fopen(FileName,"wb");
		while(1)
		{
			memset(buffer,0,sizeof(buffer));
			if(isHttps)recvcnt=SSL_read(conn,buffer,sizeof(buffer));
			else recvcnt=recv(recvSocket,buffer,sizeof(buffer),0);
			if(!isHeaderStarted)
			{
				sscanf(buffer,"HTTP/1.1 %d",&ret);
				if(ret/100==3&&allowMove)
				{
					sscanf(strstr(buffer,"Location: ")+10,"%s",location);
					isMoved=true;
				}
				isHeaderStarted=true;
			}
			if(!isContentStarted&&strstr(buffer,"Transfer-Encoding: chunked"))isChunked=true;
			if(recvcnt==SOCKET_ERROR)
			{
				ret=0;
				break;
			}
			else if(recvcnt==0)break;
			else
			{
				if(isContentStarted)
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
								if(chunksize>2)fwrite(buffer+i,1,1,fp);
								chunksize--;
							}
						}
					}
					else fwrite(buffer,1,recvcnt,fp);
				}
				else
				{
					for(i=0;i<recvcnt;++i)
					{
						state=table[(128+buffer[i])+256*state];
						if(state==4)
						{
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
										if(chunksize>2)fwrite(buffer+i,1,1,fp);
										chunksize--;
									}
								}
							}
							else fwrite(buffer+i,1,recvcnt-i,fp);
							isContentStarted=true;
							break;
						}
					}
				}
			}
		}
end:
		if(message)free(message);
		if(fp)fclose(fp);
		if(isHttps)
		{
			SSL_shutdown(conn);
			SSL_free(conn);
		}
		closesocket(recvSocket);
		WSACleanup();
		if(isMoved)
		{
			KENNYHTTPSOCKET *req=(KENNYHTTPSOCKET*)malloc(sizeof(KENNYHTTPSOCKET));
			memcpy(req,this,sizeof(KENNYHTTPSOCKET));
			req->setUrl(location);
			ret=req->responseBodyToFile(FileName,true);
			free(req);
		}
		return ret;
	}
private:
	char method[16],host[1024],path[1024],buffer[16384];
	char *header,*content;
	vector<char*> cookieName,cookieValue;
	int table[1024],headerLength,contentLength,messageLength;
	bool isHttps;
};