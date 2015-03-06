#!/usr/bin/python
from configobj import ConfigObj, flatten_errors
from validate import Validator
from flask import Flask, request, render_template, flash
from os import listdir, remove
from os.path import isfile, join

piracecam_conf = "/opt/piracecam/etc/piracecam.conf"
piracecam_spec = "/opt/piracecam/etc/piracecam.spec"
piracecam_prcd_conf = "/opt/piracecam/etc/piracecam-prcd.conf"
piracecam_space_conf = "/opt/piracecam/etc/piracecam-space.conf"
piracecam_help_conf = "/opt/piracecam/etc/piracecam-help.conf"
prc_ipaddr = ""

def write_user_config(request):
    csp = ConfigObj(piracecam_spec, interpolation=False, list_values=True, _inspec=True)
    cfg = ConfigObj(piracecam_conf, configspec=csp)
    prcdcfg = ConfigObj(piracecam_prcd_conf)
    cfgh = ConfigObj(piracecam_help_conf)
    err = ConfigObj(piracecam_conf, configspec=csp)
    validator = Validator()
    for key in cfg['UserConfigs']:
        cfg['UserConfigs'][key] = request.form[key]
    res = cfg.validate(validator, copy=True, preserve_errors=True)
    flashmsg = []
    flashmsg.append('ERROR: Please fix the invalid values in')
    if res is not True:
        for section, key, _ in flatten_errors(cfg, res):
            if key is not None:
                flashmsg.append(', ')
                flashmsg.append(key)
                for ekey in err['UserConfigs']:
                    if ekey == key:
                        err['UserConfigs'][ekey] = 'Invalid Value'
        flash(''.join(flashmsg))
        return {key:(cfg['UserConfigs'][key], csp['UserConfigs'][key], cfgh['UserConfigHelp'][key], err['UserConfigs'][key]) for key in cfg['UserConfigs']}
    else:
        cfg.write()
        f = open(prcdcfg['SystemConfigs']['PrcdSigFile'], 'w')
        f.write('reload')
        f.close()
        flash('Configuration Saved!')
    return False


def read_user_config():
    csp = ConfigObj(piracecam_spec, interpolation=False, list_values=False, _inspec=True)
    cfg = ConfigObj(piracecam_conf, configspec=csp)
    cfgh = ConfigObj(piracecam_help_conf)
    return {key:(cfg['UserConfigs'][key], csp['UserConfigs'][key], cfgh['UserConfigHelp'][key]) for key in cfg['UserConfigs']}


app = Flask(__name__)


@app.route('/settings', methods=['GET', 'POST'])
def settings():
    if request.method == 'POST':
        if request.form['submit'] == 'Save Changes':
            items = write_user_config(request)
            if items is False:
               items = read_user_config()
            return render_template('settings.html', items=items)
    else:
        items = read_user_config()
        return render_template('settings.html', items=items)



@app.route('/', methods=['GET', 'POST'])
def index():
    app.debug = True
    app.secret_key = 'neededforsomething'
    ucfg = ConfigObj(piracecam_conf)
    scfg = ConfigObj(piracecam_prcd_conf)
    mcfg = ConfigObj(piracecam_space_conf)
    if request.method == 'POST':
       if request.form['submit'] == 'REMOVE':
          ret = remove(scfg['SystemConfigs']['VFPath'] + request.form['vfid'])
          f = open(scfg['SystemConfigs']['PrcdSigFile'], 'w')
          f.write('recalc')
          f.close()
       if request.form['submit'] == 'Toggle Recording':
          f = open(scfg['SystemConfigs']['PrcdSigFile'], 'w')
          f.write('toggle')
          f.close()
    prc_ipaddr = ucfg['UserConfigs']['WiFiIP']
    strg_left = mcfg['SpaceAvailable']['MinsAvailable']
    vidfiles = [ f for f in listdir(scfg['SystemConfigs']['VFPath']) if isfile(join(scfg['SystemConfigs']['VFPath'],f)) ]
    vidfiles.sort()
    return render_template('index.html', prc_ipaddr=prc_ipaddr, files=vidfiles, strg_left=strg_left)


if __name__ == '__main__':
    nothing = False
    app.run(host='0.0.0.0', port=8080)
