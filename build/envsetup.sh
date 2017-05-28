function henv() {
cat <<EOF
Invoke ". build/envsetup.sh" from your shell to add the following functions to your environment:
- croot:   Changes directory to the top of the tree.

Look at the source to view more functions. The complete list is:
EOF
    T=$(gettop)
    local A
    A=""
    for i in `cat $T/build/envsetup.sh | sed -n "/^function /s/function \([a-z_]*\).*/\1/p" | sort`; do
      A="$A $i"
    done
    echo $A
}

function gettop
{
    local TOPFILE=build/config.mk
    if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
        echo $TOP
    else
        if [ -f $TOPFILE ] ; then
            # The following circumlocution (repeated below as well) ensures
            # that we record the true directory name and not one that is
            # faked up with symlink names.
            PWD= /bin/pwd
        else
            local HERE=$PWD
            T=
            while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
                \cd ..
                T=`PWD= /bin/pwd`
            done
            \cd $HERE
            if [ -f "$T/$TOPFILE" ]; then
                echo $T
            fi
        fi
    fi
}

function croot()
{
    T=$(gettop)
    if [ "$T" ]; then
        \cd $(gettop)
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}

############################################## dbs version makefile config ###############################
PRODUCT_TYPE=stb_dbs
echo $PRODUCT_TYPE

if [ "$PRODUCT_TYPE" = "stb_dbs" ];then
    cp $(pwd)/build/Makefile $(pwd)/Makefile -f
    sed -in-place -e 's/FLYFISH_TOP ?= $(shell pwd)/FLYFISH_TOP ?= $(shell pwd)\/platform\/linux/g'                                              $(pwd)/Makefile
    sed -in-place -e 's/FLYFISH_SDK_PATH ?= $(shell pwd)\/platform\//FLYFISH_SDK_PATH ?= $(shell pwd)\/device\/zhaoxin\/tvosh/g'                $(pwd)/Makefile
    sed -in-place -e 's/FLYFISH_HAL_PATH ?= $(shell pwd)\/platform\/hal/FLYFISH_HAL_PATH ?= $(shell pwd)\/device\/zhaoxin\/tvosh/g'             $(pwd)/Makefile
    sed -in-place -e 's/FLYFISH_HAL_INC_PATH ?= $(shell pwd)\/platform\/hal\/include/FLYFISH_HAL_INC_PATH ?= $(shell pwd)\/device\/hal\/include/g'  $(pwd)/Makefile
    sed -in-place -e 's/FLYFISH_HAL_SMP_PATH ?= $(shell pwd)\/platform\/hal\/sample/FLYFISH_HAL_SMP_PATH ?= $(shell pwd)\/device\/hal\/sample/g'    $(pwd)/Makefile
    sed -i '/include $(FLYFISH_TOP)\/cfg.mk/i\FLYFISH_APP_PATH ?= $(shell pwd)\/app'  $(pwd)/Makefile
    sed -i '/include $(FLYFISH_TOP)\/cfg.mk/a\include $(FLYFISH_TOP)\/cfg-constant.mk'  $(pwd)/Makefile
    rm $(pwd)/Makefilen-place
fi

############################################# dbs version makefile config end ############################
