#!/usr/bin/php
<?php
require_once(__DIR__ . DIRECTORY_SEPARATOR . 'Logger.php');
require_once(__DIR__ . DIRECTORY_SEPARATOR . 'BitMex.php');
require_once(__DIR__ . DIRECTORY_SEPARATOR . 'HftConnector.php');

const CONFIG_FILE_NAME = '/etc/hft/bitmex-bridge-config.json';

const POSITION_UNKNOWN = 0;
const POSITION_LONG    = 1;
const POSITION_SHORT   = 2;

function course_convert($XBTUSD) {
    $x = (int) ($XBTUSD * 10);
    return ($x / 100000);
}

function getBallance()
{
    global $bitmex;
    global $logger;

    for ($i = 0; $i < 10; $i++) {
        $a = $bitmex -> getMargin();

        if ($a !== false) {
            return $a['marginBalance'] / 100000.0;
        }

        // XXX Tutej dać warninga.
        sleep(2);
    }

    // Fatal error.
    $logger -> write(Logger::FATAL, "Cannot connect to bitmex");
    exit(1);
}

function contracts_quantity($cfg)
{
    global $logger;

    if (is_numeric($cfg)) {
        return $cfg;
    } elseif (is_array($cfg)) {
        $K0 = $cfg['K0'];
        $U  = $cfg['U'];
        $k  = $cfg['k'];
        $x  = getBallance();

        return floor($U*pow($k, log($x/$K0) / log(2)));
    }

    // Fatal error.
    $logger -> write(Logger::FATAL, "Unable to calculate contracts quantity");
    exit(1);
}

function valid_contract_per_trade($cpt)
{
    if (is_numeric($cpt) && $cpt > 0) {
        return true;
    }

    if (is_array($cpt))
    {
        foreach ([ 'K0', 'U', 'k' ] as $x) {
            if (! isset($cpt[$x]) || ! is_numeric($cpt[$x])) {
                return false;
            }
        }

        return true;
    }

    return false;
}

function load_config()
{
    global $logger;

    //
    // Example configuration:
    //
    // array(
    //    'leverage' => 1,
    //    'trade_mode' => 'test', /* docelowo bedzie też prod */
    //    'contracts_per_trade' => 1,
    //    'API' => array(
    //        'key' => 'uu6Yw37AN1tE3RU1m9AG_0xK',
    //        'secret' => 'wUd_wzwJ-L3oB5Ou3p4Vzei2pxURC8NsZZEullIqA-RGtgTv'
    //    )
    // );

    $cfg_data = file_get_contents(CONFIG_FILE_NAME);

    if ($cfg_data === NULL) {
        throw new Exception('Unable to load configuration file: '.CONFIG_FILE_NAME);
    }

    $config = json_decode($cfg_data, true);

    if ($config === NULL) {
        throw new Exception("Syntax error in json file: ".CONFIG_FILE_NAME);
    }

    //
    // Data validation.
    //

    // 1. Leverage.
    if (! isset($config['leverage'])) {
        $logger -> write(Logger::WARNING, "Undefined leverage, assume default: 1");
        $config['leverage'] = 1;
    } elseif ($config['leverage'] < 0 || $config['leverage'] > 100) {
        throw new Exception("Bad config: Leverage must be within [0..100], given ".$config['leverage']);
    }

    // 2. Trade mode.
    if (! isset($config['trade_mode'])) {
        $logger -> write(Logger::WARNING, "Undefined trade_mode, assume default [test]");
        $config['trade_mode'] = 'test';
    } elseif ($config['trade_mode'] != 'test' && $config['trade_mode'] != 'prod') {
        throw new Exception("Bad config: trade_mode must be set to prod for production or test for testnet, given ".$config['trade_mode']);
    }

    // 3. Contracts per trade.
    if (! isset($config['contracts_per_trade']) ||
            ! valid_contract_per_trade($config['contracts_per_trade'])) {
        throw new Exception("Bad config: contracts_per_trade must be positive integer value or array having K0, U, k. Given ".var_export($config['contracts_per_trade'], true));
    }

    // 4. API section (itself).
    if (! isset($config['API']) || ! is_array($config['API'])) {
        throw new Exception("Bad config: Undefined Bitmex API informations.");
    }

    // 5. API.key.
    if (! isset($config['API']['key']) || ! is_string($config['API']['key'])) {
        throw new Exception("Bad config: Undefined API.key or is defined not as a string.");
    }

    // 6. API.secret.
    if (! isset($config['API']['secret']) || ! is_string($config['API']['secret'])) {
        throw new Exception("Bad config: Undefined API.secret or is defined not as a string.");
    }

    $logger -> write(Logger::NOTICE, "User config: leverage ............. ".$config['leverage']);
    $logger -> write(Logger::NOTICE, "User config: trade mode ........... ".$config['trade_mode']);
    $logger -> write(Logger::NOTICE, "User config: contracts per trade .. ".$config['contracts_per_trade']);
    $logger -> write(Logger::NOTICE, "User config: API key .............. ".$config['API']['key']);
    $logger -> write(Logger::NOTICE, "User config: API secret ........... (defined, but snipped");

    return $config;
}

function wait_for_hft_connection_port()
{
    global $logger;

    $logger -> write(Logger::NOTICE, "Waiting for connection port information from parent process");
    $port_json = trim(fgets(STDIN));
    $json = json_decode($port_json);
    if ($json === NULL) {
        throw new Exception("Json data: [".$port_json."] cannot be decoded");
    }

    $port = $json -> {'connection_port'};

    if ($port == NULL || !is_numeric($port) || $port <= 0 || $port > 65535) {
        throw new Exception("Bad connection port value [".$port."]");
    }

    return $port;
}

function load_historical_candles($once, $qty)
{
    global $bitmex, $logger;

    $ret_candles = array();
    $now = time();

    for ($i = 0; $i < $qty; $i++) {
        $logger -> write(Logger::NOTICE, "Receiving history candles, chunk #".($i+1)." of ".$qty);

        $timestamp = $now - 60*$once*($qty - $i - 1);
        $dt = gmdate('Y-m-d H:i', $timestamp);
        $c = $bitmex -> getCandles('1m', $once, $dt);

        $ret_candles = array_merge($ret_candles, $c);
        sleep(2);
    }

    return $ret_candles;
}

function hft_tick_message(&$candle)
{
    global $logger, $hft;

    $price = course_convert($candle['close']);
    $logger -> write(Logger::NOTICE, "Received BID [".$price."] at time point [".$candle['timestamp'].']');

    $message = 'TICK;XBT/USD;'.date("Y-m-d H:i:s.000").';'.$price.";1;dummy\n";

    try {
        $response = $hft -> send_message($message);
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e -> getMessage());
        exit(1);
    }

    return $response;
}

function hft_historical_tick_message(&$candle)
{
   global $logger, $hft;

   $price = course_convert($candle['close']);
   $logger -> write(Logger::NOTICE, "Received historical BID [".$price."] at time point [".$candle['timestamp'].']');

   $message = 'HISTORICAL_TICK;XBT/USD;'.$price."\n";

    try {
        $response = $hft -> send_message($message);
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e -> getMessage());
        exit(1);
    }

    return $response;
}

/*************
**************
** Main Exec
**************
*************/

    // Current position direction - no position or it is unknown.
    $current_position_direction = POSITION_UNKNOWN;

    // Create logger.
    $logger = new Logger();
    $logger -> write(Logger::NOTICE, "Started bitmex-bridge process");

    // Load bitmex-bridge configuration.
    try {
        $config = load_config();
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e -> getMessage());
        exit(1);
    }

    // Wait for connection port to HFT parent process.
    try {
        $hft_connection_port = wait_for_hft_connection_port();
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e -> getMessage());
        exit(1);
    }

    $logger -> write(Logger::NOTICE, "Obtained connection port {$hft_connection_port} to local HFT process");

    // Create bitmex gateway.
    $bitmex = new BitMex($config['API']['key'], $config['API']['secret'], $config['trade_mode']);

    // Close all pos that could be opened eg.
    // by previous crash or VPS downtime.
    $bitmex -> closePosition(null);

    // Setup leverage.
    if ($bitmex -> setLeverage($config['leverage']) == FALSE) {
        $logger -> write(Logger::FATAL, 'Unable to setup leverage ['.$config['leverage'].']');
        exit(1);
    }

    $logger -> write(Logger::NOTICE, "Have setup leverage to {$config['leverage']}");

    // Create hft connector.
    try {
        $hft = new HftConnector($hft_connection_port);
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e->getMessage());
        exit(1);
    }

    // Subscribe instrument XBTUSD for HFT.
    $message="SUBSCRIBE;XBT/USD\n";
    try {
        $response = $hft -> send_message($message);
    } catch (Exception $e) {
        $logger -> write(Logger::FATAL, $e-> getMessage());
        exit(1);
    }

    if ($response != 'OK') {
        $logger -> write(Logger::FATAL, "Got unexpected response from HFT: {$response}");
        exit(1);
    }

    $logger -> write(Logger::NOTICE, "Subscribed instrument XBT/USD");

    // Load sufficient amount of historical candles
    // to fulfil HFT market collector.

    $start_long_op = time();

    $candles = load_historical_candles(750, 15);

    if (! is_array($candles)) {
        $logger -> write(Logger::FATAL, "Unable to obtain historical candles data");
        exit(1);
    }

    $logger -> write(Logger::NOTICE, "Got [".count($candles)."] historical candles for instrument XBT/USD");

    // Disable HCI.
//    $message = "HCI_SETUP;XBT/USD;hci_off\n";
//    try {
//        $response = $hft -> send_message($message);
//    } catch (Exception $e) {
//        $logger -> write(Logger::FATAL, $e-> getMessage());
//        exit(1);
//    }

    foreach ($candles as $prev_candle) {
        $response = hft_historical_tick_message($prev_candle);

        switch ($response) {
            case 'OK':
                break;
            default:
                $logger -> write(Logger::FATAL, "Got unexpected response from HFT: [".$response.']');
                exit(1);
        }
    }

    // Free memory.
    unset($candles);

    $end_long_op = time();

    $logger -> write(Logger::NOTICE, "Finished acquisit & applying data after ". ($end_long_op - $start_long_op) ." seconds.");

    // Enable HCI.
//    $message = "HCI_SETUP;XBT/USD;hci_on\n";
//    try {
//        $response = $hft -> send_message($message);
//    } catch (Exception $e) {
//        $logger -> write(Logger::FATAL, $e-> getMessage());
//        exit(1);
//    }

    // Operational.
    while (true) {
        // Obtain last candle.
        while (true) {
            $cur_candle = $bitmex -> getCandles('1m', 1);
            if ($cur_candle == null) {
                $logger -> write(Logger::WARNING, "Unable to get candle, will retry");
                sleep(2);
                continue;
            }

            if ($prev_candle != $cur_candle) {
                $prev_candle = $cur_candle;
                break;
            } else {
                sleep(10);
            }
        }

        // Reload bitmex-bridge configuration
        // that user might change in meanwhile.
        try {
            $tmp_config = load_config();

            if ($tmp_config['leverage'] != $config['leverage']) {
                // Change leverage.
                if ($bitmex -> setLeverage($tmp_config['leverage']) == FALSE) {
                    $logger -> write(Logger::ERROR, 'Unable to setup leverage ['.$tmp_config['leverage'].']');
                }
            }

            $config = $tmp_config;
        } catch (Exception $e) {
            $logger -> write(Logger::ERROR, "Bad config: " . $e -> getMessage());
        }

        $candle = $cur_candle;
        $candle = array_pop($candle);

        $response = hft_tick_message($candle);
        $tick = $bitmex -> getTicker();

        switch ($response) {
            case 'OK':
                // Nothing to do
                break;
            case 'LONG':
                $logger -> write(Logger::NOTICE, "HFT requested to open LONG position, buying: [".$config['contracts_per_trade'].'] contracts');
                // $bitmex -> createOrder(null, 'Buy', null, $config['contracts_per_trade']);
                $bitmex -> createOrder('Limit', 'Buy', $tick['ask'] - 1, $config['contracts_per_trade']);
                $current_position_direction = POSITION_LONG;
                break;
            case 'SHORT':
                $logger -> write(Logger::NOTICE, "HFT requested to open SHORT position, selling: [".$config['contracts_per_trade'].'] contracts');
                // $bitmex -> createOrder(null, 'Sell', null, $config['contracts_per_trade']);
                $bitmex -> createOrder('Limit', 'Sell', $tick['bid'] + 1, $config['contracts_per_trade']);
                $current_position_direction = POSITION_SHORT;
                break;
            case 'CLOSE':
                $logger -> write(Logger::NOTICE, "HFT requested close position");
                switch ($current_position_direction) {
                    case POSITION_LONG:
                        // FIXME: For now we close position at price market anyway.
                        $bitmex -> closePosition(null);
                        break;
                    case POSITION_SHORT:
                        // FIXME: For now we close position at price market anyway.
                        $bitmex -> closePosition(null);
                        break;
                    default:
                        // No chance, we've to close
                        // position at the market price.
                        $bitmex -> closePosition(null);
                        break;
                }
                break;
            default:
                $logger -> write(Logger::FATAL, "Got unexpected response from HFT: [".$response.']');
                exit(1);
        }

   } /* while (true) */
