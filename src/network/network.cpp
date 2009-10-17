#include "hawknl/nl.h"
#include "network.h"
#include "globals.h"
#include <string>
#include <sstream>

using namespace std;

/* TODO: Wrap open_sockets with a mutex */
namespace Network{

NetworkException::~NetworkException() throw (){
}
	
InvalidPortException::InvalidPortException( int port, const string message ):
NetworkException(""){
	ostringstream num;
	num << port;
	num << ". ";
	num << message;
	this->setMessage( "Invalid port " + num.str() );
}

Message::Message(){
	memset( data, 0, sizeof(data) );
	position = data;
}
	
Message::Message( const Message & m ){
	memcpy( data, m.data, sizeof(data) );
	position = data;
	position += m.position - m.data;
	path = m.path;
	id = m.id;
}
	
Message & Message::operator=( const Message & m ){

	memcpy( data, m.data, sizeof(data) );
	position = data;
	position += m.position - m.data;
	path = m.path;
	id = m.id;

	return *this;
}

Message::Message( Socket socket ){
	position = data;
	id = read16( socket );
	readBytes( socket, data, DATA_SIZE );
	int str = read16( socket );
	if ( str != -1 ){
		char buf[ 1024 ];
		str = (signed)(sizeof( buf ) - 1) < str ? (signed)(sizeof(buf) - 1) : str;
		readBytes( socket, (uint8_t *) buf, str );
		buf[ str ] = 0;
		this->path = buf;
	}
}

uint8_t * Message::dump( uint8_t * buffer ) const {
	*(uint16_t *) buffer = id;
	buffer += sizeof(uint16_t);
	memcpy( buffer, data, DATA_SIZE );
	buffer += DATA_SIZE;
	if ( path != "" ){
		*(uint16_t *) buffer = path.length() + 1;
		buffer += sizeof(uint16_t);
		memcpy( buffer, path.c_str(), path.length() + 1 );
		buffer += path.length() + 1;
	} else {
		*(uint16_t *) buffer = (uint16_t) -1;
		buffer += sizeof(uint16_t);
	}
	return buffer;
}

/*
template <typename M>
int messageSize(const M& message);
*/

/*
template <>
int messageSize<Message>(Message const & message){
    return message.size();
}

template <>
int messageSize<Message*>(Message* const & message){
    return message->size();
}
*/
int messageSize(Message const & message){
    return message.size();
}

int messageSize(Message* const & message){
    return message->size();
}

/*
template <class M>
uint8_t * messageDump(const M& message, uint8_t * buffer);

template <>
uint8_t * messageDump<Message>(const Message & message, uint8_t * buffer){
    return message.dump(buffer);
}

template <>
uint8_t * messageDump<Message*>(Message* const & message, uint8_t * buffer){
    return message->dump(buffer);
}
*/
uint8_t * messageDump(const Message & message, uint8_t * buffer){
    return message.dump(buffer);
}

uint8_t * messageDump(Message* const & message, uint8_t * buffer){
    return message->dump(buffer);
}

template <typename M>
int totalSize(const vector<M> & messages){
    int size = 0;
    for (typename vector<M>::const_iterator it = messages.begin(); it != messages.end(); it++){
        // size += messageSize<M>(*it);
        size += messageSize(*it);
    }
    return size;
}

template <class M>
void dump(const std::vector<M> & messages, uint8_t * buffer ){
    for (typename vector<M>::const_iterator it = messages.begin(); it != messages.end(); it++ ){
        buffer = messageDump(*it, buffer);
    }
}

template <class M>
void doSendAllMessages(const vector<M> & messages, Socket socket){
    int length = totalSize<M>(messages);
    uint8_t * data = new uint8_t[length];
    dump<M>(messages, data);
    sendBytes(socket, data, length);
    delete[] data;
}

void sendAllMessages(const vector<Message> & messages, Socket socket){
    doSendAllMessages<Message>(messages, socket);
}

void sendAllMessages(const vector<Message*> & messages, Socket socket){
    doSendAllMessages<Message*>(messages, socket);
}

void Message::send( Socket socket ) const {
	/*
	send16( socket, id );
	sendBytes( socket, data, DATA_SIZE );
	if ( path != "" ){
		send16( socket, path.length() + 1 );
		sendStr( socket, path );
	} else {
		send16( socket, -1 );
	}
	*/
	uint8_t * buffer = new uint8_t[ size() ];
	dump( buffer );
	sendBytes( socket, buffer, size() );
	delete[] buffer;
}
	
void Message::reset(){
	position = data;
}

Message & Message::operator<<(int x){

	*(int16_t *) position = x;
	position += sizeof(int16_t);

	return *this;
}

Message & Message::operator<<(unsigned int x){
	*(int32_t *) position = x;
	position += sizeof(int32_t);
	return *this;
}
	
Message & Message::operator>>(int & x){
	x = *(int16_t *) position;
	position += sizeof(int16_t);
	return *this;
}

Message & Message::operator>>(unsigned int & x){
	x = *(int32_t *) position;
	position += sizeof(int32_t);
	return *this;
}

Message & Message::operator<<( string p ){
	path = p;
	return *this;
}
	
int Message::size() const {
	return sizeof( uint16_t ) + DATA_SIZE + 
		(path != "" ? sizeof( uint16_t ) + path.length() + 1 :
		sizeof( uint16_t ));
}

static string getHawkError(){
	return string(" HawkNL error: '") +
	       string( nlGetErrorStr( nlGetError() ) ) +
	       string( "' HawkNL system error: '" ) +
	       string( nlGetSystemErrorStr( nlGetSystemError() ) );
}

int16_t read16( Socket socket ){
	uint8_t data[ sizeof(uint16_t) ];
	readBytes( socket, data, sizeof(uint16_t) ); 
	return *(uint16_t *)data;
	/*
	uint16_t b;
	int read = nlRead( socket, &b, sizeof(int16_t) );
	if ( read != sizeof(int16_t) ){
		throw NetworkException( string("Could not read 16 bits.") + getHawkError() );
	}
	return b;
	*/
}

void send16( Socket socket, int16_t bytes ){
	/*
	if ( nlWrite( socket, &length, sizeof(int16_t) ) != sizeof(int16_t) ){
		throw NetworkException( string("Could not send 16 bits.") + getHawkError() );
	}
	*/
	sendBytes( socket, (uint8_t *) &bytes, sizeof(bytes) );
}

string readStr( Socket socket, const uint16_t length ){

	char buffer[ length + 1 ];
	NLint bytes = nlRead( socket, buffer, length );
	if ( bytes == NL_INVALID ){
		throw NetworkException( string("Could not read string.") + getHawkError() );
	}
	buffer[ length ] = 0;
	bytes += 1;
	return string( buffer );

}

void sendStr( Socket socket, const string & str ){
	if ( nlWrite( socket, str.c_str(), str.length() + 1 ) != (signed)(str.length() + 1) ){
		throw NetworkException( string("Could not write string.") + getHawkError() );
	}
}

void sendBytes( Socket socket, const uint8_t * data, int length ){
	const uint8_t * position = data;
	int written = 0;
	while ( written < length ){
		/* put htons here for endianess compatibility */
		int bytes = nlWrite( socket, position, length - written );
		if ( bytes == NL_INVALID ){
			throw NetworkException( string("Could not send bytes.") + getHawkError() );
		}
		written += bytes;
		position += bytes;
	}
}

void readBytes( Socket socket, uint8_t * data, int length ){
	uint8_t * position = data;
	int read = 0;
	while ( read < length ){
		/* put htons here for endianess compatibility */
		int bytes = nlRead( socket, position, length - read );
		if ( bytes == NL_INVALID ){
			throw NetworkException( string("Could not read bytes.") + getHawkError() );
		}
		read += bytes;
		position += bytes;
	}
}

Socket open( int port ) throw( InvalidPortException ){
	// NLsocket server = nlOpen( port, NL_RELIABLE_PACKETS );
	Global::debug( 1 ) << "Attemping to open port " << port << endl;
	Socket server = nlOpen( port, NL_RELIABLE );
	if ( server == NL_INVALID ){
		throw InvalidPortException(port, nlGetSystemErrorStr(nlGetSystemError()));
	}
	open_sockets.push_back( server );
	return server;
}

Socket connect( string server, int port ) throw ( NetworkException ) {
	NLaddress address;
	nlGetAddrFromName( server.c_str(), &address );
	nlSetAddrPort( &address, port );
	Socket socket = open( 0 );
	if ( nlConnect( socket, &address ) == NL_FALSE ){
		close( socket );
		throw NetworkException( "Could not connect" );
	}
	return socket;
}

void close( Socket s ){
	for ( vector< Socket >::iterator it = open_sockets.begin(); it != open_sockets.end(); ){
		if ( *it == s ){
			Global::debug( 1 ) << "Closing socket " << s << endl;
			nlClose( *it );
			Global::debug( 1 ) << "Closed" << endl;
			it = open_sockets.erase( it );
		} else {
			it++;
		}
	}
}

void closeAll(){
	Global::debug( 0 ) << "Closing all sockets" << std::endl;
	for ( vector< Socket >::iterator it = open_sockets.begin(); it != open_sockets.end(); it++ ){
		nlClose( *it );
	}
	open_sockets.clear();
}

void init(){
	nlInit();
	nlSelectNetwork( NL_IP );
	nlEnable( NL_BLOCKING_IO );
	// nlDisable( NL_BLOCKING_IO );
}

void blocking( bool b ){
	if ( b ){
		nlEnable( NL_BLOCKING_IO );
	} else {
		nlDisable( NL_BLOCKING_IO );
	}
}

void listen( Socket s ) throw( NetworkException ){
	if ( nlListen( s ) == NL_FALSE ){
		throw CannotListenException( string(nlGetSystemErrorStr( nlGetSystemError() )) );
	}
}

Socket accept( Socket s ) throw( NetworkException ){
	Socket connection = nlAcceptConnection( s );
	if ( connection == NL_INVALID ){
		/*
		if ( nlGetError() == NL_NO_PENDING ){
			error = NO_CONNECTIONS_PENDING;
		} else {
			error = NETWORK_ERROR;
		}
		return s;
		*/
		if ( nlGetError() == NL_NO_PENDING ){
			throw NoConnectionsPendingException();
		}
		throw NetworkException("Could not accept connection");
	}
	open_sockets.push_back( connection );
	return connection;
}

void shutdown(){
	nlShutdown();
}

}
