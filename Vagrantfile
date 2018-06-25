# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|

  config.vm.box = "ubuntu/trusty64"

  config.vm.provider "virtualbox" do |vb|
    vb.cpus = 2
    vb.memory = 2048
  end
  
  config.ssh.username = 'vagrant'
  config.ssh.password = 'vagrant'
  config.ssh.insert_key = 'true'

  config.vm.define "php-master" do |box|
    box.vm.network :forwarded_port, guest: 22, host: 2222, id: "ssh"
    box.vm.provision "shell", path: "vagrant-php.sh"
  end
  
end
