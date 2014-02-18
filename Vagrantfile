Vagrant.configure("2") do |config|
    config.vm.define "wheezy64" do |c|
        c.vm.box = "wheezy64"
        c.vm.provision :shell, :path => "res/bootstrap-debian.sh"
        c.vm.synced_folder ".", "/home/vagrant/nordlicht"
    end
    config.vm.define "gentoo64" do |c|
        c.vm.box = "gentoo64"
    end
end
