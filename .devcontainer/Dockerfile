FROM retcon85/toolchain-sms:0.13

# install ohmyzsh
USER root
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install zsh
RUN zsh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
USER retcon
