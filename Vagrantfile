Vagrant.configure("2") do |config|
    config.vm.define "ubuntu" do |c|
        c.vm.box = "ubuntu"
        c.vm.box_url = "http://files.vagrantup.com/precise64.box"
        c.vm.provision :shell, :path => "utils/bootstrap-debian.sh"
        c.vm.synced_folder ".", "/home/vagrant/nordlicht"
        c.ssh.forward_agent = true
    end
    config.vm.define "debian" do |c|
        c.vm.box = "debian"
        c.vm.box_url = "https://dl.dropboxusercontent.com/u/197673519/debian-7.2.0.box"
        c.vm.provision :shell, :path => "utils/bootstrap-debian.sh"
        c.vm.synced_folder ".", "/home/vagrant/nordlicht"
        c.ssh.forward_agent = true
    end
    config.vm.define "gentoo" do |c|
        c.vm.box = "gentoo"
        c.vm.box_url = "https://dl.dropboxusercontent.com/s/0e23qmbo97wb5x2/gentoo-20131029-i686-minimal.box"
    end
    config.vm.define "suse" do |c|
        c.vm.box = "suse"
        c.vm.box_url = "http://sourceforge.net/projects/opensusevagrant/files/12.2/opensuse-12.2-64.box/download"
    end
end
