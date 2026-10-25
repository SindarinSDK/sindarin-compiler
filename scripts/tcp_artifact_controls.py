"""Independent TCP API, field and lifetime programs; public SDK source is untouched."""
BACKING = r'''#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
bool descriptorClosed(long long descriptor) {
#ifdef _WIN32
    int kind=0,length=sizeof(kind);
    int status=getsockopt((SOCKET)descriptor,SOL_SOCKET,SO_TYPE,(char *)&kind,&length);
    return status==SOCKET_ERROR && WSAGetLastError()==WSAENOTSOCK;
#else
    return fcntl((int)descriptor,F_GETFD)==-1 && errno==EBADF;
#endif
}
long long inputState(void) {
#ifdef _WIN32
    return (long long)GetFileType(GetStdHandle(STD_INPUT_HANDLE));
#else
    return fcntl(0,F_GETFD);
#endif
}
long long makeWake(void) {
#ifdef _WIN32
    SOCKET fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (fd==INVALID_SOCKET) abort();
#else
    int fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (fd<0) abort();
#endif
    struct sockaddr_in address={0};address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(fd,(struct sockaddr *)&address,sizeof(address))) abort();
#ifdef _WIN32
    int length=sizeof(address);
#else
    socklen_t length=sizeof(address);
#endif
    if (getsockname(fd,(struct sockaddr *)&address,&length)) abort();
    if (sendto(fd,"x",1,0,(struct sockaddr *)&address,sizeof(address))!=1) abort();
    return (long long)fd;
}
void closeWake(long long fd) {
#ifdef _WIN32
    if (closesocket((SOCKET)fd)) abort();
#else
    if (close((int)fd)) abort();
#endif
}
static long long effect_count;
long long nextEffect(void) { return ++effect_count; }
long long effectCount(void) { return effect_count; }
'''
COMMON = '''@source "control.sn.c"
import "sindarin-pkg-sdk/src/net/tcp"
native fn descriptorClosed(fd: int): bool
native fn inputState(): int
native fn makeWake(): int
native fn closeWake(fd: int): void
'''
API = COMMON + '''native fn noStream(): TcpStream =>
  return nil
native fn noListener(): TcpListener =>
  return nil
native fn defaults(): bool =>
  var stream: TcpStream = TcpStream {_socket_fd:0,_remote_addr:nil,_internal:nil}
  var listener: TcpListener = TcpListener {_socket_fd:0,_bound_port:0}
  return stream._socket_fd==0 && stream._remote_addr==nil && stream._internal==nil && listener._socket_fd==0 && listener._bound_port==0
fn releasedListener(): int =>
  var listener: TcpListener = TcpListener.bind(":0")
  return listener._socket_fd
fn releasedAccepted(): int =>
  var listener: TcpListener = TcpListener.bind(":0")
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = listener.accept()
  return server._socket_fd
fn escapedBytes(): byte[] =>
  var listener: TcpListener = TcpListener.bind(":0")
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = listener.accept()
  client.write({0b,128b,255b,42b})
  client.dispose()
  return server.readAll()
fn escapedLine(): str =>
  var listener: TcpListener = TcpListener.bind(":0")
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = listener.accept()
  client.writeLine("é漢")
  return server.readLine()
fn escapedAddress(): str =>
  var listener: TcpListener = TcpListener.bind(":0")
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = listener.accept()
  return server.remoteAddress()
fn rawRoundtrip(): bool =>
  var listener: TcpListener = sn_tcp_listener_bind(":0")
  var client: TcpStream = sn_tcp_stream_connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = sn_tcp_listener_accept(listener)
  client.writeLine("raw")
  var line: str = sn_tcp_stream_read_line(server)
  var remote: str = sn_tcp_stream_get_remote_address(server)
  var empty: byte[] = sn_tcp_stream_read(server,0)
  client.write({0b,128b,255b})
  sn_tcp_stream_dispose(client)
  var data: byte[] = sn_tcp_stream_read_all(server)
  sn_tcp_stream_dispose(client)
  sn_tcp_stream_dispose(server)
  sn_tcp_listener_dispose(listener)
  return line=="raw" && remote.startsWith("127.0.0.1:") && empty.length==0 && data.length==3 && data[0]==0b && data[1]==128b && data[2]==255b
fn main(): void =>
  var before: int = inputState()
  println(defaults())
  println(inputState()==before)
  var missing: TcpStream = noStream()
  println(!missing.isValid())
  println(missing.read(0).length==0 && missing.readAll().length==0 && missing.readLine()=="")
  println(missing.remoteAddress()=="" && missing.write({1b})==0)
  missing.setTimeout(0)
  missing.writeLine("ignored")
  missing.dispose()
  var missingListener: TcpListener = noListener()
  println(missingListener.port()==0)
  missingListener.dispose()
  println(descriptorClosed(releasedListener()))
  println(descriptorClosed(releasedAccepted()))
  var data: byte[] = escapedBytes()
  println(data.length==4 && data[0]==0b && data[1]==128b && data[2]==255b && data[3]==42b)
  data.push(9b)
  println(data.length==5 && data[4]==9b)
  println(escapedLine()=="é漢")
  println(escapedAddress().startsWith("127.0.0.1:"))
  println(rawRoundtrip())
  var listener: TcpListener = TcpListener.bind(":0")
  var aliasListener: TcpListener = listener
  println(aliasListener._socket_fd==listener._socket_fd && listener._bound_port==listener.port())
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{listener.port()}")
  var server: TcpStream = listener.accept()
  var alias: TcpStream = server
  println(alias._socket_fd==server._socket_fd && alias._internal==server._internal && server._remote_addr!=nil)
  alias.setTimeout(0)
  println(alias.read(4).length==0)
  alias.setTimeout(2000)
  client.writeLine("shared")
  println(server.readLine()=="shared")
  client.dispose()
  client.dispose()
  println(!client.isValid() && server.read(4).length==0 && server.readAll().length==0)
  alias.dispose()
  println(!server.isValid() && server._socket_fd == -1 && server._internal==nil && server._remote_addr==nil)
  aliasListener.dispose()
  println(listener._socket_fd == -1)
  var signalListener: TcpListener = TcpListener.bind(":0")
  var wakeFD: int = makeWake()
  var sentinel: TcpStream = signalListener.acceptOrShutdown(wakeFD)
  println(!sentinel.isValid() && sentinel.readAll().length==0)
  closeWake(wakeFD)
  signalListener.dispose()
  var rawListener: TcpListener = sn_tcp_listener_bind(":0")
  var rawWake: int = makeWake()
  var rawSentinel: TcpStream = sn_tcp_listener_accept_or_shutdown(rawListener,rawWake)
  println(!rawSentinel.isValid())
  closeWake(rawWake)
  sn_tcp_stream_dispose(rawSentinel)
  sn_tcp_listener_dispose(rawListener)
'''
FIELDS = COMMON + '''fn listenerRef(value: TcpListener as ref): int =>
  value._bound_port=17
  return value._bound_port
fn streamRef(value: TcpStream as ref): int =>
  value._socket_fd = -1
  return value._socket_fd
fn listenerDefault(value: TcpListener): int =>
  value._bound_port=31
  return value._bound_port
struct FieldCalls as ref =>
  marker: int
  static fn staticStore(value: TcpListener): int =>
    value._bound_port=37
    return value._bound_port
  fn instanceStore(value: TcpListener as ref): int =>
    value._bound_port=41
    return value._bound_port
fn main(): void =>
  var listener: TcpListener = TcpListener.bind(":0")
  var port: int = listener.port()
  println(listenerRef(listener)==17 && listener.port()==17)
  listener._bound_port=port
  var snapshot: TcpListener = listener
  snapshot._bound_port=29
  println(listener.port()==29 && snapshot.port()==29)
  println(listenerDefault(listener)==31 && snapshot.port()==31)
  var callable = listenerRef
  println(callable(listener)==17 && snapshot.port()==17)
  println(FieldCalls.staticStore(listener)==37 && snapshot.port()==37)
  var calls: FieldCalls = {marker:0}
  println(calls.instanceStore(listener)==41 && snapshot.port()==41)
  listener._bound_port=port
  var client: TcpStream = TcpStream.connect($"127.0.0.1:{port}")
  var server: TcpStream = listener.accept()
  var fd: int = client._socket_fd
  println(streamRef(client) == -1 && client._socket_fd == -1)
  client._socket_fd=fd
  var copied: TcpStream = client
  copied._socket_fd = -1
  println(client._socket_fd == -1 && !copied.isValid())
  client._socket_fd=fd
  client.dispose()
  server.dispose()
  listener.dispose()
'''
EFFECTS = COMMON + '''native fn nextEffect(): int
native fn effectCount(): int
struct EffectCalls as ref =>
  marker: int
  static fn staticValue(): int =>
    return nextEffect()
  fn instanceValue(): int =>
    return nextEffect()
fn ordinary(): int =>
  return nextEffect()
fn main(): void =>
  var unreadStatic: int = EffectCalls.staticValue()
  println(effectCount()==1)
  var calls: EffectCalls = {marker:0}
  var unreadInstance: int = calls.instanceValue()
  println(effectCount()==2)
  var unreadDirect: int = ordinary()
  println(effectCount()==3)
  var unreadBinary: int = 1+ordinary()
  println(effectCount()==4)
  var unreadArray: int[] = {ordinary()}
  println(effectCount()==5)
'''
CONTROLS = [('api-lifetimes', API, ('releasedListener','releasedAccepted','escapedBytes','escapedLine','escapedAddress','rawRoundtrip')),
            ('reference-field-aliases', FIELDS, ('listenerRef','streamRef','listenerDefault')),
            ('unused-initializer-effects', EFFECTS, ('ordinary',))]


def rejected_copy(record, operation):
    if operation=='as-val':
        body=f'fn copied(value: {record} as val): int =>\n  return value._socket_fd\n'
        expression='copied(value)'
    else:
        body=''
        expression='copyOf(value)._socket_fd'
    factory='TcpListener.bind(":0")' if record=='TcpListener' else 'TcpStream.connect("ignored")'
    return COMMON+body+f'fn main(): void =>\n  var value: {record} = {factory}\n  println({expression})\n'
