#!/bin/sh


pybabel extract -F babel.config -o lang/messages.pot .

#pybabel update -l de_DE -d ./lang -i ./lang/messages.pot
pybabel update -l en_US -d ./lang -i ./lang/messages.pot
#pybabel update -l es_VE -d ./lang -i ./lang/messages.pot
pybabel update -l fa_IR -d ./lang -i ./lang/messages.pot
#pybabel update -l ja_JP -d ./lang -i ./lang/messages.pot
pybabel update -l zh_CN -d ./lang -i ./lang/messages.pot

#msgfmt lang/zh_CN/LC_MESSAGES/messages.po -o lang/zh_CN/LC_MESSAGES/messages.mo 
sed -i -e '/#.*$/d'  lang/en_US/LC_MESSAGES/messages.po
sed -i -e '/#.*$/d'  lang/fa_IR/LC_MESSAGES/messages.po
sed -i -e '/#.*$/d'  lang/zh_CN/LC_MESSAGES/messages.po
