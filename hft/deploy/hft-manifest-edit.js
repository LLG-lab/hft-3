#!/usr/bin/node

/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2020 by LLG Ryszard Gradowski          **
**                       All Rights Reserved.                         **
**                                                                    **
**  CAUTION! This application is an intellectual propery              **
**           of LLG Ryszard Gradowski. This application as            **
**           well as any part of source code cannot be used,          **
**           modified and distributed by third party person           **
**           without prior written permission issued by               **
**           intellectual property owner.                             **
**                                                                    **
\**********************************************************************/

var path = require('path');
var fs   = require('fs');

const instrument_dir='/etc/hft/';

function exit_error(message, rc = 1) {
    if (process.stderr.isTTY) {
        console.error("\033[;31m** ERROR **\033[0m " + message);
    } else {
        console.error("** ERROR ** " + message);
    }

    process.exit(rc);
}

function show_usage() {
    console.log('');
    console.log(path.basename(process.argv[1]) + ' – Instrument manifest JSON data manipulation utility');
    console.log('Copyright  2017 - 2020 by LLG Ryszard Gradowski, All Rights Reserved.');
    console.log('');
    console.log('Usage:');
    console.log('  ' + path.basename(process.argv[1])+' <instrument> option[=value] [option[=value] ...]');
    console.log('');
    console.log('Options:');
    console.log('');
    console.log('  Usage info');
    console.log('    --help (or -h)              Show this help.');
    console.log('');
    console.log('  HCI controller related');
    console.log('    --disable-hci               Disables HCI, if enabled.');
    console.log('    --enable-hci                Enables HCI with default settings,');
    console.log('                                if disabled.');
    console.log('    --hci-capacity=INTVAL       Setup capacity for HCI. HCI controller');
    console.log('                                must be prior enabled.');
    console.log('    --hci-s-threshold=INTVAL    Setup straight threshold for HCI.');
    console.log('                                HCI controller must be prior enabled.');
    console.log('    --hci-i-threshold=INTVAL    Setup invert threshold for HCI.');
    console.log('                                HCI controller must be prior enabled.');
    console.log('');
    console.log('  Pips goal limits');
    console.log('    --dpips-limit-profit=INTVAL Setup deci-pips limit for profit.');
    console.log('    --dpips-limit-loss=INTVAL   Setup deci-pips limit for loss.');
    console.log('');
    console.log('  Neural networks & approximator related');
    console.log('    --architecture=STRING       Setup neural network(s) architecture.');
    console.log('    --collector-size=INTVAL     Market collector capacity.');
    console.log('    --vote-ratio=DOUBLEVAL      Decision vote ratio for neural');
    console.log('                                networks (0.5 < vote-ratio ≤ 1.0).');
    console.log('    --approximation=STRING      Setup JSON file name with binomial ');
    console.log('                                aproximation informations.');
    console.log('    --granularity=INTVAL        Setup tick’s granularity for market');
    console.log('                                collector.');
    console.log('');
}

function should_help(a) {
    for (var i = 2; i < process.argv.length; i++) {
        if (process.argv[i] == '--help' || process.argv[i] == '-h') {
            return true;
        }
    }

    return false;
}

function validate_instrument(instr) {
    const alpha = [ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0',
                    '1', '2', '3', '4', '5', '6', '7', '8', '9' ];

    if (instr.length != 6) {
        exit_error('Bad financial instrument: ‘' + instr + '’');
    }

    for (var i = 0; i < instr.length; i++) {
        if (! alpha.includes(instr[i])) {
            exit_error('Bad financial instrument: ‘' + instr + '’');
        }
    }

    return instr;
}

function parse_params() {
    var ret = [];

    for (var i = 3; i < process.argv.length; i++) {
        var x = process.argv[i].split('=');

        if (x.length > 2) {
            exit_error('Bad syntax for argument: ‘' + process.argv[i] + '’');
        }

        var u = {
            'param' : x[0],
            'value' : x[1]
        };

        ret.push(u);
    }

    return ret;
}

function load_manifest(instr) {
    var file_name = instrument_dir + instr + '/manifest.json';
    var json_data = '';

    try{
        json_data = fs.readFileSync(file_name).toString('utf8');
    } catch (e) {
        exit_error('Fatal error ‘' + e.code + '’ while loadning manifest file');
    }

    try {
        return JSON.parse(json_data);
    } catch (e) {
        exit_error('JSON parsing error of ‘' + file_name + '’ - ' + e);
    }
}

function save_manifest(instr, json) {
    var file_name = instrument_dir + instr + '/manifest.json';
    var json_data = JSON.stringify(json, null, 4);

    try {
        fs.writeFileSync(file_name, json_data, 'utf8');
    } catch (e) {
        exit_error('Fatal error ‘' + e.code + '’ while saving manifest file');
    }
}

function manipulate(j, op) {
    var v;

    switch (op.param) {
        case '--disable-hci'  :
            if (j.hysteresis_compensate_inverter != null) {
                j.hysteresis_compensate_inverter = undefined;
            }

            break;
        case '--enable-hci' :
            if (j.hysteresis_compensate_inverter === undefined) {
                j.hysteresis_compensate_inverter = { capacity: 3, s_threshold: -1, i_threshold: 1 };
            }

            break;
        case '--hci-capacity' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (v <= 0) {
                exit_error('Negative value is not allowed for parameter ‘' + op.param + '’', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            if (j.hysteresis_compensate_inverter === undefined) {
                exit_error('Cannot set capacity for HCI - HCI controller is disabled for instrument ‘' + instrument + '’', 2);
            }
            j.hysteresis_compensate_inverter.capacity = v;

            break;
        case '--hci-s-threshold' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            if (j.hysteresis_compensate_inverter === undefined) {
                exit_error('Cannot set straight threshold for HCI - HCI controller is disabled for instrument ‘' + instrument + '’', 2);
            }
            j.hysteresis_compensate_inverter.s_threshold = v;

            break;
        case '--hci-i-threshold' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            if (j.hysteresis_compensate_inverter === undefined) {
                exit_error('Cannot set invert threshold for HCI - HCI controller is disabled for instrument ‘' + instrument + '’', 2);
            }
            j.hysteresis_compensate_inverter.i_threshold = v;

            break;
        case '--dpips-limit-profit' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (v <= 0) {
                exit_error('Negative value is not allowed for parameter ‘' + op.param + '’', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.dpips_limit_profit = v;

            break;
        case '--dpips-limit-loss' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (v <= 0) {
                exit_error('Negative value is not allowed for parameter ‘' + op.param + '’', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.dpips_limit_loss = v;

            break;
        case '--architecture' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = op.value.trim();
            if (v.length < 3 || v[0] != '[' || v[v.length - 1] != ']') {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.architecture = v;

            break;
        case '--collector-size' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (v <= 0) {
                exit_error('Negative value is not allowed for parameter ‘' + op.param + '’', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.collector_size = v;

            break;
        case '--vote-ratio' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseFloat(op.value);
            if (v <= 0.5 || v > 1.0) {
                exit_error('Parameter value for‘' + op.param + '’ must be within (0.5 ... 1.0].', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.vote_ratio = v;

            break;
        case '--approximation' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = op.value.trim();
            if (v.length == 0) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.binomial_approximation = v;

            break;
        case '--granularity' :
            if (op.value == null) {
                exit_error('Unspecified parameter value for ‘' + op.param + '’', 2);
            }
            v = parseInt(op.value);
            if (v <= 0) {
                exit_error('Negative value is not allowed for parameter ‘' + op.param + '’', 2);
            } else if (isNaN(v)) {
                exit_error('Bad value ‘' + op.value  + '’ for parameter ‘' + op.param + '’', 2);
            }
            j.granularity = v;

            break;
        default:
            exit_error('Illegal operation ‘' + op.param + '’', 2);
    }
}

/***************
** Main Exec
***************/

if (process.argv.length == 2) {
    show_usage();
    exit_error("No arguments specified", 1);
} else if (should_help()) {
    show_usage();
    process.exit(0);
}

var instrument = validate_instrument(process.argv[2].toUpperCase());
var args = parse_params();

var j = load_manifest(instrument);

for (var i = 0; i < args.length; i++) {
    manipulate(j, args[i]);
}

save_manifest(instrument, j);
