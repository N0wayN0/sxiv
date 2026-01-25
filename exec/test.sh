rows=$(tput lines)
cols=$(tput cols)

bottom_lines=20

start_line=$((rows-bottom_lines +1))

echo -e "\033[2j"

read -n 1
echo -e "\033[${start_line};0H"

for ((i=0;i<cols;i++));do
echo -n "-"
done
read -n 1

echo
echo -e "\033[$((start_line+1));0H"

read -n 1
vim

echo -e "\033[0m"
