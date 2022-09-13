# Get the base Ubuntu image from Docker Hub
FROM centos:8
# FROM continuumio/anaconda3:latest

LABEL maintainer="jian.weng@ucla.edu"
ENV DEBIAN_FRONTEND=noninteractive 

# Update apps on the base image
# Packages
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-Linux-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-Linux-*
RUN yum makecache --refresh
RUN yum update -y
RUN yum groupinstall -y "Development Tools"
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y gperf texinfo expat-devel util-linux-user
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

