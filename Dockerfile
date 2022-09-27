# Get the base centos8 image from Docker Hub
FROM --platform=linux/amd64 centos:8

LABEL maintainer="Sihao Liu <sihao@cs.ucla.edu>; Jian Weng<jian.weng@ucla.edu>"
ENV DEBIAN_FRONTEND=noninteractive 

# Update apps on the base image
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-Linux-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-Linux-*
RUN dnf makecache --refresh
RUN dnf update -y

# Install packages
RUN dnf groupinstall -y "Development Tools"
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y gperf texinfo expat-devel util-linux-user dtc
RUN dnf install -y autoconf automake curl bison flex libtool gmp-devel ncurses-devel \
		   patchutils bc flex bison java-11-openjdk-devel libpng-devel perl \
		   libmpc-devel mpfr zlib-devel zip unzip zsh tmux wget git vim emacs gettext
RUN dnf install -y https://rpmfind.net/linux/opensuse/distribution/leap/15.3/repo/oss/x86_64/fd-8.1.1-bp153.1.14.x86_64.rpm
RUN dnf update -y

# Install verilator
RUN cd /root && git clone http://git.veripool.org/git/verilator && cd verilator && \
    git checkout v4.034 && autoconf && ./configure && make -j$(nproc) && make install && \
    rm -rf /root/verilator

# SBT for Scala
RUN rm -f /etc/yum.repos.d/bintray-rpm.repo &&                       \
    curl -L https://www.scala-sbt.org/sbt-rpm.repo > sbt-rpm.repo && \
    mv sbt-rpm.repo /etc/yum.repos.d/ &&                             \
    dnf install -y sbt
RUN dnf update -y

# Oh-my-zsh
RUN sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

# Install Anaconda3
RUN cd /root && wget https://repo.anaconda.com/archive/Anaconda3-2022.05-Linux-x86_64.sh && \
    chmod u+x ./Anaconda3-2022.05-Linux-x86_64.sh && \
    ./Anaconda3-2022.05-Linux-x86_64.sh -b -p /root/anaconda3
RUN dnf update -y

# Initialize Anaconda3 under zsh
RUN echo "source /root/anaconda3/bin/activate" >> /root/.zshrc
RUN dnf update -y

# Download the repos
RUN cd /root && git clone https://github.com/polyarch/dsa-framework
RUN cd /root/dsa-framework && ./scripts/init-submodules.sh
RUN dnf update -y

# Remove Anaconda install file
RUN rm /root/Anaconda3-2022.05-Linux-x86_64.sh
RUN dnf update -y
