nofomatdisk=$(parted -l |grep "unrecognised"|awk '{print $3}' |cut -d: -f1)

#####fomat the disk
if [[ -n $nofomatdisk ]];
then
#if nofomatdisk is not none
#then fomat the disk
mkfs.ext4 $nofomatdisk << EOF
y
EOF
fi

####mount the disk
mounted=$(sfdisk -s |grep dev |awk '{print $1}' |cut -d: -f1)
for diskname in $(sfdisk -s |grep dev |awk '{print $1}' |cut -d: -f1);
do
ret=$(mount |grep $diskname)
if [[ -z $ret ]];
then
#if there is has a disk is not mounted
name=${diskname##*/}
rootdir='/'
dirname=${rootdir}${name}
mkdir $dirname
mount $diskname $dirname
fi
done
