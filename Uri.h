// based on http://stackoverflow.com/a/11044337

#ifndef URI_H_
#define URI_H_

#include "application.h"

struct Uri
{
    public:
        String QueryString, Path, Protocol, Host, Port;

        static Uri Parse(String uri)
        {
            Uri result;

            if (uri.length() == 0)
            {
                return result;
            }

            int uriEnd = uri.length();

            // get query start
            int queryStart = uri.indexOf("?", 0);

            // protocol
            int protocolStart = 0;
            int protocolEnd = uri.indexOf(":", 0);

            if (protocolEnd != uriEnd)
            {
                String prot = uri.substring(protocolEnd);
                if (prot.length() > 3 && (prot.substring(0, 3) == "://"))
                {
                    String protocol = uri.substring(protocolStart, protocolEnd + 3);
                    result.Protocol = protocol;
                    protocolEnd += 3;
                }
                else
                {
                    protocolEnd = 0; // no protocol
                }
            }
            else
            {
                protocolEnd = 0; // no protocol
            }

            // host
            int hostStart = protocolEnd;
            int pathStart = uri.indexOf("/", hostStart); // get pathStart

            int hostEnd = uri.indexOf(":", protocolEnd); // check for port
            if (hostEnd == -1)
            {
                hostEnd = (pathStart != uriEnd) ? pathStart : queryStart;
            }

            String host = uri.substring(hostStart, hostEnd);
            result.Host = host;

            // port
            if (hostEnd != uriEnd && uri.substring(hostEnd)[0] == ':')
            {
                hostEnd++;
                int portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
                String port = uri.substring(hostEnd, portEnd);
                result.Port = port;
            }

            // path
            if (pathStart != uriEnd)
            {
                String path = uri.substring(pathStart, queryStart);
                result.Path = path;
            }

            // query
            if (queryStart != uriEnd)
            {
                String querystring = uri.substring(queryStart, uriEnd);
                result.QueryString = querystring;
            }

            return result;
        }   // Parse
};  // uri

#endif
