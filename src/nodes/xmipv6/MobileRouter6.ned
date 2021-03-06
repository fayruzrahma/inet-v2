//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

// fayruz 27.01.2015
// mobile router is derived from the combined behaviors of: host, router, mobile node [RFC 3963 p.9]
// 		host 		--> extends NodeBase6
// 		router 		--> IPForward = true
//		mobile node --> xmipv6 support 

package inet.nodes.xmipv6;

import inet.linklayer.INic;
import inet.linklayer.IWiredNic;
import inet.networklayer.xmipv6.PrefixTable;
import inet.nodes.ipv6.StandardHost6;

//
// IPv6 router.
//
// see ~Router for configuration options
//
module MobileRouter6 extends StandardHost6
{
    parameters:
        @display("i=abstract/router;bgb=731,479");
        IPForward = true;



//        numRadios = 1;

        networkLayer.xMIPv6Support = true;
        networkLayer.mipv6support.isMobileNode = false; //is Mobile Node
        networkLayer.mipv6support.isHomeAgent = false; //is Home Agent
        networkLayer.mipv6support.isMobileRouter = true; //is Mobile Router

    gates:
        inout pintu;

//    submodules:
//        iWiredNic: <"ForwardInterface"> like IWiredNic {
//            @display("p=667,406");
//        }
//    connections allowunconnected:
//        pintu <--> iWiredNic.phys;
//        networkLayer.ifOut++ --> iWiredNic.upperLayerIn;
//        networkLayer.ifIn++ <-- iWiredNic.upperLayerOut;

    submodules: //double:: di xMIPv6Support juga ada ini!
        prefixTable: PrefixTable {
            @display("p=53,54");
        }
}

