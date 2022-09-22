# Get the base Ubuntu image from Docker Hub
FROM centos:8

LABEL maintainer="jian.weng@ucla.edu;sihao@cs.ucla.edu"
ENV DEBIAN_FRONTEND=noninteractive 

# Update apps on the base image
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-Linux-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-Linux-*
RUN yum makecache --refresh
RUN yum update -y

# Install packages
RUN yum groupinstall -y "Development Tools"
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y gperf texinfo expat-devel util-linux-user dtc
RUN yum install -y autoconf automake curl bison flex libtool gmp-devel ncurses-devel \
		   patchutils bc flex bison java-11-openjdk-devel libpng-devel perl \
		   libmpc-devel mpfr zlib-devel zip unzip zsh tmux wget git vim emacs gettext

# SBT for Scala
RUN rm -f /etc/yum.repos.d/bintray-rpm.repo &&                       \
    curl -L https://www.scala-sbt.org/sbt-rpm.repo > sbt-rpm.repo && \
    mv sbt-rpm.repo /etc/yum.repos.d/ &&                             \
    yum install -y sbt

# Oh-my-zsh
RUN sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

# Install Anaconda3
RUN cd /root && wget https://repo.anaconda.com/archive/Anaconda3-2022.05-Linux-x86_64.sh && \
    chmod u+x ./Anaconda3-2022.05-Linux-x86_64.sh && \
    ./Anaconda3-2022.05-Linux-x86_64.sh -b -p /root/anaconda3

# Initialize Anaconda3 under zsh
RUN echo "source /root/anaconda3/bin/activate" >> /root/.zshrc

# Download the repos
RUN cd /root && git clone https://github.com/polyarch/dsa-framework
RUN cd /root/dsa-framework && ./scripts/init-submodules.sh

# Remove Anaconda install file
RUN rm /root/Anaconda3-2022.05-Linux-x86_64.sh
