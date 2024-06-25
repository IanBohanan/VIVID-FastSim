FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential vim git iputils-ping wget systemd tcpdump net-tools libpcap-dev
RUN git config --global url.https://fastsim:XxZ8iijDJ4FNjTHvz5Lf@gitlab.uah.edu/.insteadOf "https://gitlab.uah.edu/"
RUN git clone -b Attack-Files https://gitlab.uah.edu/fastsim/FastSim.git
RUN apt install -y \ 
iptables \
openssh-server
RUN apt-get install -y \
kmod \
hping3 \
python3-pip \
scapy \
build-essential python3-dev libnetfilter-queue-dev
RUN python3 -m pip install pyModbusTCP
RUN python3 -m pip install cython
RUN git clone https://github.com/oremanj/python-netfilterqueue
RUN cd python-netfilterqueue && python3 setup.py install
RUN rm -rf python-netfilterqueue
RUN wget -P /usr/local/ https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
RUN cd /usr/local && tar --bzip2 -xf ./boost_1_79_0.tar.bz2
RUN rm -f boost_1_79_0.tar.bz2
RUN cd /FastSim/OpenPLC_v3 && ./install.sh docker