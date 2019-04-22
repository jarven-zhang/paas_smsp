ROOT_DIR = ..
TARGET_DIR = $(ROOT_DIR)/build/smsp_plugin/lib
PLUGIN_DIR = $(ROOT_DIR)/source/smsp_plugin
SMSP_DIR = $(ROOT_DIR)/build
#LIB_SO = eosm fzd gd gy gzdy itrigo jxhttp mw ngd ssdj synivers wlwx xjcz yf ytwl zycc cxdy jytd wt  ymrt shhttp mcwx ypw emasym testin xyc xxxx bstsx sdzy \
#zssdhttp jxtemplatehttp yxdy ljlz jxtempnew yunmas yunmastemp jccs qxt dxtemplate zjyd txy mas dcg cjxx xxd shsj hljw cz rh xhr tx

LIB_SO = gd  gzdy itrigo jxhttp mw ngd ssdj synivers wlwx xjcz yf ytwl cxdy jytd wt  ymrt shhttp mcwx xyc xxxx sdzy \
zssdhttp yxdy ljlz jccs dcg cjxx xxd shsj hljw xhr tx

.PHONY = help all clean cleanall $(LIB_SO)

help:
	@(echo "usage: make help|all|clean|cleanall|install|eosm|fzd|gd|gy|gzdy|itrigo|jxhttp|mw|ngd|ssdj|synivers|wlwx|xjcz|yf|ytwl|zycc|cxdy|jytd|xyc|xxxx|zssdhttp|jxtemplatehttp|ljlz|bstsx|sdzy|jccs|qxt|dxtemplate|zjyd|txy|mas|dcg|cjxx|xxd|shsj")

cz:
	@(cd $(PLUGIN_DIR); make lib=cz)

xhr:
	@(cd $(PLUGIN_DIR); make lib=xhr)

tx:
	@(cd $(PLUGIN_DIR); make lib=tx)

hl:
	@(cd $(PLUGIN_DIR); make lib=hl)

rh:
	@(cd $(PLUGIN_DIR); make lib=rh)

hljw:
	@(cd $(PLUGIN_DIR); make lib=hljw)

ch:
	@(cd $(PLUGIN_DIR); make lib=ch)

stx:	
	@(cd $(PLUGIN_DIR); make lib=stx)

mas:	
	@(cd $(PLUGIN_DIR); make lib=mas)

#eosm:
#	@(cd $(PLUGIN_DIR); make lib=eosm)
fzd:
	@(cd $(PLUGIN_DIR); make lib=fzd)
gd:
	@(cd $(PLUGIN_DIR); make lib=gd)
gy:
	@(cd $(PLUGIN_DIR); make lib=gy)
gzdy:
	@(cd $(PLUGIN_DIR); make lib=gzdy)
itrigo:
	@(cd $(PLUGIN_DIR); make lib=itrigo)
jxhttp:
	@(cd $(PLUGIN_DIR); make lib=jxhttp)
mw:
	@(cd $(PLUGIN_DIR); make lib=mw)
ngd:
	@(cd $(PLUGIN_DIR); make lib=ngd)
ssdj:
	@(cd $(PLUGIN_DIR); make lib=ssdj)
synivers:
	@(cd $(PLUGIN_DIR); make lib=synivers)
wlwx:
	@(cd $(PLUGIN_DIR); make lib=wlwx)
xjcz:
	@(cd $(PLUGIN_DIR); make lib=xjcz)
yf:
	@(cd $(PLUGIN_DIR); make lib=yf)
ytwl:
	@(cd $(PLUGIN_DIR); make lib=ytwl)
zycc:
	@(cd $(PLUGIN_DIR); make lib=zycc)
cxdy:
	@(cd $(PLUGIN_DIR); make lib=cxdy)
jytd:
	@(cd $(PLUGIN_DIR); make lib=jytd)
wt:
	@(cd $(PLUGIN_DIR); make lib=wt)
ymrt:
	@(cd $(PLUGIN_DIR); make lib=ymrt)
shhttp:
	@(cd $(PLUGIN_DIR); make lib=shhttp)
mcwx:
	@(cd $(PLUGIN_DIR); make lib=mcwx)
ypw:
	@(cd $(PLUGIN_DIR); make lib=ypw)
emasym:
	@(cd $(PLUGIN_DIR); make lib=emasym)
testin:
	@(cd $(PLUGIN_DIR); make lib=testin)
xyc:
	@(cd $(PLUGIN_DIR); make lib=xyc)
xxxx:
	@(cd $(PLUGIN_DIR); make lib=xxxx)
zssdhttp:
	@(cd $(PLUGIN_DIR); make lib=zssdhttp)
jxtemplatehttp:
	@(cd $(PLUGIN_DIR); make lib=jxtemplatehttp)
yxdy:
	@(cd $(PLUGIN_DIR); make lib=yxdy)
ljlz:
	@(cd $(PLUGIN_DIR); make lib=ljlz)
jxtempnew:
	@(cd $(PLUGIN_DIR); make lib=jxtempnew)
yunmas:
	@(cd $(PLUGIN_DIR); make lib=yunmas)
bstsx:
	@(cd $(PLUGIN_DIR); make lib=bstsx)
sdzy:
	@(cd $(PLUGIN_DIR); make lib=sdzy)
yunmastemp:
	@(cd $(PLUGIN_DIR); make lib=yunmastemp)
jccs:
	@(cd $(PLUGIN_DIR); make lib=jccs)
yqx:
	@(cd $(PLUGIN_DIR); make lib=yqx)
dxtemplate:
	@(cd $(PLUGIN_DIR); make lib=dxtemplate)
zjyd:
	@(cd $(PLUGIN_DIR); make lib=zjyd)
txy:
	@(cd $(PLUGIN_DIR); make lib=txy)
dcg:
	@(cd $(PLUGIN_DIR); make lib=dcg)
cjxx:
	@(cd $(PLUGIN_DIR); make lib=cjxx)
xxd:
	@(cd $(PLUGIN_DIR); make lib=xxd)
shsj:
	@(cd $(PLUGIN_DIR); make lib=shsj)
rh:
	@(cd $(PLUGIN_DIR); make lib=rh)
all:$(LIB_SO)

install:
	@(echo "")
	@(echo "begin copy file...")
	cp -f $(TARGET_DIR)/* $(SMSP_DIR)/smsp_send/lib/
	cp -f $(TARGET_DIR)/* $(SMSP_DIR)/smsp_report/lib/
	
clean:
	@(cd $(PLUGIN_DIR); make clean)

cleanall:
	@(cd $(PLUGIN_DIR); make clean)
	rm -rf $(TARGET_DIR)/*




