FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential vim git iputils-ping wget systemd tcpdump net-tools libpcap-dev libboost-all-dev
RUN git config --global url.https://fastsim:REPLACE_WITH_PASSKEY@github.com/.insteadOf "https://github.com/"
RUN git clone -b main https://github.com/IanBohanan/VIVID-FastSim
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
RUN apt-get -y install sudo
RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo
RUN python3 -m pip install cython
RUN git clone https://github.com/oremanj/python-netfilterqueue
RUN cd python-netfilterqueue && python3 setup.py install
RUN rm -rf python-netfilterqueue
RUN wget -P /usr/local https://boostorg.jfrog.io/artifactory/main/release/1.79.0/source/boost_1_79_0.tar.bz2
RUN cd /usr/local && tar --bzip2 -xf ./boost_1_79_0.tar.bz2
RUN export BOOST_ROOT=/usr/local/boost_1_79_0
RUN ifconfig -V
RUN echo $BOOST_ROOT
RUN echo CHECK_ABOVE_LINE_IF_BOOST_INSTALLED!!!!
RUN chmod -R 777 VIVID-FastSim
RUN cd /VIVID-FastSim/OpenPLC_v3 && ./install.sh docker
