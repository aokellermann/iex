# Based off of https://github.com/archlinux/archlinux-docker.git

FROM scratch
ADD archlinux.tar /
ENV LANG=en_US.UTF-8
CMD ["/usr/bin/bash"]

RUN pacman -Sy --noconfirm sudo base base-devel
RUN useradd -m antony
RUN echo "antony ALL=(ALL) NOPASSWD:ALL" | sudo tee -a /etc/sudoers

USER antony
WORKDIR /home/antony

RUN sudo pacman -S --noconfirm gcc cmake clang git curl nlohmann-json
RUN git clone https://aur.archlinux.org/cpplint.git
WORKDIR /home/antony/cpplint
RUN makepkg --noconfirm -si
WORKDIR /home/antony