server compilation:
                `g++ main.cpp dbvpnserver.cpp -o dbvpnserver -ltins`

server start:
		        `./dbvpnserver <tun_local_ip/10.10.10.10> <snat_port_range/5000-6000> <tun_address/10.9.8.7> <if_name/wlp1s0>`

client start:
			`sudo ./dbvpnclient <tun_local_ip/10.82.166.166> <port/5000> <tun_address/10.8.0.2> <if_name/wlp1s0>`
