#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <memory.h>

extern "C" {
#include <libircclient.h>
}

struct DCCFile
{
    irc_dcc_t id;
    std::string filename;
    unsigned long size;
    unsigned long received;
    std::ofstream stream;

    DCCFile(irc_dcc_t id, const char *filename, unsigned long size) 
	:   id(id),
	    filename(filename),
	    size(size),
	    received(0)
    { stream.open(filename, std::fstream::binary); }
};

typedef std::shared_ptr<DCCFile> DCCFilePtr;

struct IRCContext
{
    std::vector<std::string> channels;
    std::string nick;
    std::map<irc_dcc_t, std::shared_ptr<DCCFile>> files;

};

void event_connect(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
    IRCContext* ctx = (IRCContext*) irc_get_ctx(session);

    for (auto &c : ctx->channels)
    {
        irc_cmd_join (session, c.c_str(), 0);
    }
}

void event_channel(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
    char nickbuf[128];

    if ( count != 2 )
    return;

    std::cout << origin << "@" << params[0] << ": " << params[1] << std::endl;

    if ( !origin )
    return;
}

void dcc_file_recv_callback (irc_session_t* session, irc_dcc_t id, int status, void* fp, const char* data, unsigned int length)
{
    IRCContext* ctx = (IRCContext*) irc_get_ctx(session);
    DCCFilePtr fileptr = ctx->files[id];

    if (status == 0 && length == 0)
    {
	std::cout << "File " << fileptr->filename << " received successfully!" << std::endl;
	fileptr->stream.close();
	ctx->files.erase(id);
    }
    else if (status)
    {
	std::cout << "Error receiving file '" << fileptr->filename << "': " << status << std::endl;
	fileptr->stream.close();
	ctx->files.erase(id);
    }
    else
    {
	fileptr->received += length;
	float percent = ((float)fileptr->received / (float)fileptr->size) * 100.0f;
	std::cout << "File '" << fileptr->filename << "': "
		  << std::setprecision(4) << percent << "%"
		  << " (" << fileptr->received << "/" << fileptr->size << ")" << std::endl;
	fileptr->stream.write(data, length);
    }
}

void event_dcc_send_req(irc_session_t* session, const char* nick, const char* addr, const char* filename, unsigned long size, irc_dcc_t dccid)
{

    IRCContext* ctx = (IRCContext*) irc_get_ctx(session);
    boost::filesystem::path p(filename);
    DCCFilePtr fileptr = std::make_shared<DCCFile>(dccid, filename, size);

    ctx->files[dccid] = fileptr;

    std::cout << "Receiving file " << filename << ", size: " << size << std::endl;

    irc_dcc_accept(session, dccid, nullptr, dcc_file_recv_callback);
}

int main(int argc, char **argv)
{
    irc_callbacks_t    callbacks;
    IRCContext ctx;
    irc_session_t * s;
    unsigned short port = 6667;

    if ( argc < 4 )
    {
        printf ("Usage: %s <server> <nick> <channel>\n", argv[0]);
        return 1;
    }

    memset (&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_channel = event_channel;
    //callbacks.event_privmsg = event_privmsg;

    callbacks.event_dcc_send_req = event_dcc_send_req;

    s = irc_create_session (&callbacks);

    if ( !s )
    {
        printf ("Could not create session\n");
        return 1;
    }

    ctx.nick = argv[2];
    unsigned int channel = 3;

    while (channel < argc)
    {
        ctx.channels.push_back(argv[channel]);
        channel++;
    }

    irc_set_ctx (s, &ctx);

    // If the port number is specified in the server string, use the port 0 so it gets parsed
    if ( strchr( argv[1], ':' ) != 0 )
        port = 0;

    // To handle the "SSL certificate verify failed" from command line we allow passing ## in front 
    // of the server name, and in this case tell libircclient not to verify the cert
    if ( argv[1][0] == '#' && argv[1][1] == '#' )
    {
        // Skip the first character as libircclient needs only one # for SSL support, i.e. #irc.freenode.net
        argv[1]++;
        
        irc_option_set( s, LIBIRC_OPTION_SSL_NO_VERIFY );
    }
    
    // Initiate the IRC server connection
    if ( irc_connect (s, argv[1], port, 0, argv[2], 0, 0) )
    {
        printf ("Could not connect: %s\n", irc_strerror (irc_errno(s)));
        return 1;
    }

    // and run into forever loop, generating events
    if ( irc_run (s) )
    {
        printf ("Could not connect or I/O error: %s\n", irc_strerror (irc_errno(s)));
        return 1;
    }

    return 1;
}
