/*
 *  AMQP.cpp
 *  librabbitmq++
 *
 *  Created by Alexandre Kalendarev on 01.03.10.
 *
 */

#include "AMQPcpp.h"

AMQP::AMQP() {
	AMQP::init();
	AMQP::initDefault();
	AMQP::connect();
};

AMQP::AMQP(string cnnStr) {
	AMQP::init();
	AMQP::parseCnnString(cnnStr);
	AMQP::connect();
};

AMQP::~AMQP() {
	if (channels.size()) {
		vector<AMQPBase*>::iterator i;
		for (i=channels.begin(); i!=channels.end(); i++) {
			delete *i;
		}
	}

	amqp_destroy_connection(cnn);
	close(sockfd);
};

void AMQP::init() {
	exchange=NULL;
	channelNumber=0;
}

void AMQP::initDefault() {
	host = AMQPHOST;
	port = AMQPPORT;
	vhost = AMQPVHOST;
	user = AMQPLOGIN;
	password = AMQPPSWD;
}

void AMQP::parseCnnString( string cnnString ) {
	 if (!cnnString.size()) {
		AMQP::initDefault();
		return;
	 }

	// find '@' if Ok -> right part is host:port else all host:port
	string hostPortStr, userPswString;
	string::size_type pos = cnnString.find('@');

	switch (pos) {
		case 0:
			hostPortStr.assign(cnnString, 1, string::npos);
			AMQP::parseHostPort(hostPortStr);
			user = AMQPLOGIN;
			password = AMQPPSWD;
		break;
		case string::npos:
			AMQP::parseHostPort(cnnString);
			user = AMQPLOGIN;
			password = AMQPPSWD;
		break;
		default :
			hostPortStr.assign(cnnString, pos+1, string::npos);
			userPswString.assign(cnnString, 0, pos);
			AMQP::parseHostPort(hostPortStr);
			AMQP::parseUserStr(userPswString );
		break;
	}
}

void AMQP::parseUserStr(string userString) {
	string::size_type pos = userString.find(':');
	switch (pos) {
		case 0:
			user = AMQPLOGIN;
			password.assign(userString, 1, string::npos);
		break;
		case string::npos:
			user = userString;
			password = AMQPPSWD;
		break;
		default:
			user.assign(userString, 0, pos);
			password.assign(userString, pos+1, string::npos);
		break;
	}
}

void AMQP::parseHostPort(string hostPortString ) {
	string hostString;
	string portString;

	string::size_type pos = hostPortString.find(':');
	string::size_type pos2 = hostPortString.find('/');

	host = AMQPHOST;
	vhost = AMQPVHOST;
	port = AMQPPORT;

	if (pos == string::npos) {
		if ( pos2 == string::npos) {
			host = hostPortString;
		} else {
			vhost.assign(hostPortString, pos2+1, string::npos);
			if (pos2 > 0) {
				host.assign(hostPortString, 0, pos2);
			}
		}
	} else if (pos == 0) {
		if (pos2 == string::npos) {
			portString.assign(hostPortString, 1, string::npos);
		} else {
			portString.assign(hostPortString, 1, pos2-1);
			vhost.assign(hostPortString, pos2+1, string::npos);
		}
		port = atoi(portString.c_str());
	} else {
		if ( pos2 == string::npos ) {
			host.assign(hostPortString, 0, pos);
			portString.assign(hostPortString, pos+1, string::npos);
		} else {
			vhost.assign(hostPortString, pos2+1, string::npos);
			host.assign(hostPortString, 0, pos);
			portString.assign(hostPortString, pos+1, pos2-pos-1);
		}
		port = atoi(portString.c_str());
	}
}

void AMQP::connect() {
	AMQP::sockConnect();
	AMQP::login();
}

void AMQP::printConnect() {
	 cout<<  "AMQP connection: \n";

	 cout<<  "port  = " << port << endl;
	 cout<<  "host  = " << host << endl;
	 cout<<  "vhost = " << vhost << endl;
	 cout<<  "user  = " << user << endl;
	 cout<<  "passw = " << password << endl;
}

void AMQP::sockConnect() {
	cnn = amqp_new_connection();
	sockfd = amqp_open_socket(host.c_str(), port);

	if (sockfd<0){
		amqp_destroy_connection(cnn);
		throw AMQPException("AMQP cannot create socket descriptor");
	}

	//cout << "sockfd="<< sockfd  << "  pid=" <<  getpid() <<endl;
	amqp_set_sockfd(cnn, sockfd);
}

void AMQP::login() {
	amqp_rpc_reply_t res = amqp_login(cnn, vhost.c_str(), 0, FRAME_MAX, 0, AMQP_SASL_METHOD_PLAIN, user.c_str(), password.c_str());
	if ( res.reply_type == AMQP_RESPONSE_NORMAL)
		return;

	amqp_destroy_connection(cnn);
	throw AMQPException(&res);
}

AMQPExchange * AMQP::createExchange() {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber);
	channels.push_back( dynamic_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPExchange * AMQP::createExchange(string name) {
	channelNumber++;
	AMQPExchange * exchange = new AMQPExchange(&cnn,channelNumber,name);
	channels.push_back( dynamic_cast<AMQPBase*>(exchange) );
	return exchange;
}

AMQPQueue * AMQP::createQueue() {
	channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber);
	channels.push_back( dynamic_cast<AMQPBase*>(queue) );
	return queue;
}

AMQPQueue * AMQP::createQueue(string name) {
        channelNumber++;
	AMQPQueue * queue = new AMQPQueue(&cnn,channelNumber,name);
	channels.push_back( dynamic_cast<AMQPBase*>(queue) );
	return queue;
}

void AMQP::closeChannel() {
	channelNumber--;
	AMQPBase * cnn = channels.back();
	if (cnn) {
		delete cnn;
		channels.pop_back();
	}
}
