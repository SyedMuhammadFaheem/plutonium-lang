#ifndef SOCKET_PLT_H_
#define SOCKET_PLT_H_
#ifdef _WIN32
#include "C:\plutonium\PltObject.h"
#define EXPORT __declspec(dllexport)
#endif
#define EXPORT 0
extern "C"
{
	//Functions
	EXPORT PltObject init();
	//Methods
	EXPORT PltObject socket__construct(PltObject*, int);
	EXPORT PltObject socket_Bind( PltObject*,int);
	EXPORT PltObject socket_Connect( PltObject*,int);
	EXPORT PltObject socket_Send( PltObject*,int);
	EXPORT PltObject socket_Recv( PltObject*,int);
	EXPORT PltObject socket_Listen( PltObject*,int);
	EXPORT PltObject socket_Accept( PltObject*,int);
	EXPORT PltObject socket_Close( PltObject*,int);
	EXPORT PltObject socket_SendTo( PltObject*, int);
	EXPORT PltObject socket_RecvFrom( PltObject*, int);
	EXPORT PltObject socket_Destroy( PltObject*, int);
}
#endif