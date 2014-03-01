Vagrant.configure("2") do |config|
    config.vm.define "precise64" do |c|
        c.vm.box = "precise64"
        c.vm.box_url = "http://files.vagrantup.com/precise64.box"
        c.vm.provision :shell, :path => "utils/bootstrap-debian.sh"
        c.vm.synced_folder ".", "/home/vagrant/nordlicht"
        c.ssh.forward_agent = true
    end
    config.vm.define "wheezy64" do |c|
        c.vm.box = "wheezy64"
        c.vm.box_url = "https://dl.dropboxusercontent.com/u/197673519/debian-7.2.0.box"
        c.vm.provision :shell, :path => "utils/bootstrap-debian.sh"
        c.vm.synced_folder ".", "/home/vagrant/nordlicht"
        c.ssh.forward_agent = true
    end
    config.vm.define "gentoo64" do |c|
        c.vm.box = "gentoo64"
        c.vm.box_url = "https://dl.dropboxusercontent.com/s/0e23qmbo97wb5x2/gentoo-20131029-i686-minimal.box"
    end
end
