/**********************************************************************\
**                                                                    **
**             -=≡≣ High Frequency Trading System  ≣≡=-              **
**                                                                    **
**          Copyright  2017 - 2021 by LLG Ryszard Gradowski          **
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

package hft2ducascopy;

import com.dukascopy.api.Instrument;
import com.dukascopy.api.system.ClientFactory;
import com.dukascopy.api.system.IClient;
import com.dukascopy.api.system.ISystemListener;
import java.nio.file.Files;
import java.nio.charset.Charset;
import java.nio.file.Paths;
//import org.json.simple.parser.ParseException;
//import org.json.simple.parser.JSONParser;

import org.json.*;

import java.util.HashSet;
import java.util.Set;
import java.io.*;

public class Main {

    private static String jnlpUrl_demo = "http://platform.dukascopy.com/demo/jforex.jnlp";
    private static String jnlpUrl_live = "http://platform.dukascopy.com/live_3/jforex_3.jnlp";
    private static String jnlpUrl = "";

    private static String userName = "username";
    private static String password = "password";
    private static JSONObject config;

    private static IClient client;

    private static int lightReconnects = 3;

    private static String readFile(String path, Charset encoding) throws IOException
    {
        byte[] encoded = Files.readAllBytes(Paths.get(path));
        return new String(encoded, encoding);
    }

    private static JSONObject load_configuration() {
        try
        {
            JSONObject cfg = new JSONObject(readFile("/etc/hft/dukascopy-bridge-config.json", Charset.defaultCharset()));
            return cfg;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            System.err.println("Failed to load configuration: " + e.getMessage());
            System.exit(1);
        } catch (IOException e) {
            e.printStackTrace();
            System.err.println("Failed to load configuration: " + e.getMessage());
            System.exit(1);
        }

        return new JSONObject();
    }

    public static void main(String[] args) throws Exception {

        System.out.println("Waiting for connection port information...");

        BufferedReader buffer = new BufferedReader(new InputStreamReader(System.in));
        String cfg_str = buffer.readLine();

        int port = 0;

        try
        {
            JSONObject port_config = new JSONObject(cfg_str);

            port = port_config.getInt("connection_port");
        }
        catch (JSONException e)
        {
            System.err.println("Connection port configuration ERROR: " + e.getMessage());
            System.exit(1);
        }

        //
        // Other settings are in json configuration file.
        //

        try
        {
            config = load_configuration();

            if (config.getString("mode").equals("live"))
            {
                System.out.println("Starting with mode LIVE");
                jnlpUrl = jnlpUrl_live;

                userName = config.getJSONObject("authorization_live").getString("username");
                password = config.getJSONObject("authorization_live").getString("password");
            }
            else if (config.getString("mode").equals("demo"))
            {
                System.out.println("Starting with mode DEMO");
                jnlpUrl = jnlpUrl_demo;

                userName = config.getJSONObject("authorization").getString("username");
                password = config.getJSONObject("authorization").getString("password");
            }
            else
            {
                System.err.println("Invalid mode " + config.getString("mode"));
                System.exit(1);
            }
        }
        catch (JSONException e)
        {
            System.err.println("Failed to load configuration: " + e.getMessage());
            System.exit(1);
        }

        //get the instance of the IClient interface
        client = ClientFactory.getDefaultInstance();

        setSystemListener();

        System.out.println("Connecting...");

        tryToConnect();

        if (! client.isConnected()) {
            System.err.println("Failed to connect Dukascopy servers");
            System.exit(1);
        }

        subscribeToInstruments();

        System.out.println("Starting Proxy Bridge operations");

        client.startStrategy(new ProxyBridge(port, config));
        //now it's running
    }

    private static void setSystemListener() {
        //set the listener that will receive system events
        client.setSystemListener(new ISystemListener() {

            @Override
            public void onStart(long processId) {
                System.out.println("Strategy started: " + processId);
            }

            @Override
            public void onStop(long processId) {
                System.out.println("Strategy stopped: " + processId);
                if (client.getStartedStrategies().size() == 0) {
                    System.exit(0);
                }
            }

            @Override
            public void onConnect() {
                System.out.println("Connected");
                lightReconnects = 3;
            }

            @Override
            public void onDisconnect() {
                tryToReconnect();
            }
        });
    }

    private static void tryToConnect() throws Exception {
        //connect to the server using jnlp, user name and password
        client.connect(jnlpUrl, userName, password);

        //wait for it to connect
        int i = 10; //wait max ten seconds
        while (i > 0 && !client.isConnected()) {
            Thread.sleep(1000);
            i--;
        }
    }

    private static void tryToReconnect() {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                while (! client.isConnected()) {
                    try {
                        if (lightReconnects > 0 && client.isReconnectAllowed()) {
                            client.reconnect();
                            --lightReconnects;
                        } else {
                            tryToConnect();
                        }
                        if(client.isConnected()) {
                            break;
                        }
                    } catch (Exception e) {
                        System.err.println(e.getMessage());
                    }
                    sleep(60 * 1000);
                }
            }

            private void sleep(long millis) {
                try {
                    Thread.sleep(millis);
                } catch (InterruptedException e) {
                    System.err.println(e.getMessage());
                }
            }
        };
        new Thread(runnable).start();
    }

    private static void subscribeToInstruments() {
        Set<Instrument> instruments = new HashSet<>();

        if (config.has("AUDUSD"))
        {
            System.out.println("Subscribing AUDUSD");
            instruments.add(Instrument.AUDUSD);
        }

        if (config.has("EURUSD"))
        {
            System.out.println("Subscribing EURUSD");
            instruments.add(Instrument.EURUSD);
        }

        if (config.has("GBPUSD"))
        {
            System.out.println("Subscribing GBPUSD");
            instruments.add(Instrument.GBPUSD);
        }

        if (config.has("NZDUSD"))
        {
            System.out.println("Subscribing NZDUSD");
            instruments.add(Instrument.NZDUSD);
        }

        if (config.has("USDCAD"))
        {
            System.out.println("Subscribing USDCAD");
            instruments.add(Instrument.USDCAD);
        }

        if (config.has("USDCHF"))
        {
            System.out.println("Subscribing USDCHF");
            instruments.add(Instrument.USDCHF);
        }

        if (config.has("USDJPY"))
        {
            System.out.println("Subscribing USDJPY");
            instruments.add(Instrument.USDJPY);
        }

        if (config.has("USDCNH"))
        {
            System.out.println("Subscribing USDCNH");
            instruments.add(Instrument.USDCNH);
        }

        if (config.has("USDDKK"))
        {
            System.out.println("Subscribing USDDKK");
            instruments.add(Instrument.USDDKK);
        }

        if (config.has("USDHKD"))
        {
            System.out.println("Subscribing USDHKD");
            instruments.add(Instrument.USDHKD);
        }

        if (config.has("USDHUF"))
        {
            System.out.println("Subscribing USDHUF");
            instruments.add(Instrument.USDHUF);
        }

        if (config.has("USDMXN"))
        {
            System.out.println("Subscribing USDMXN");
            instruments.add(Instrument.USDMXN);
        }

        if (config.has("USDNOK"))
        {
            System.out.println("Subscribing USDNOK");
            instruments.add(Instrument.USDNOK);
        }

        if (config.has("USDPLN"))
        {
            System.out.println("Subscribing USDPLN");
            instruments.add(Instrument.USDPLN);
        }

        if (config.has("USDRUB"))
        {
            System.out.println("Subscribing USDRUB");
            instruments.add(Instrument.USDRUB);
        }

        if (config.has("USDSEK"))
        {
            System.out.println("Subscribing USDSEK");
            instruments.add(Instrument.USDSEK);
        }

        if (config.has("USDSGD"))
        {
            System.out.println("Subscribing USDSGD");
            instruments.add(Instrument.USDSGD);
        }

        if (config.has("USDTRY"))
        {
            System.out.println("Subscribing USDTRY");
            instruments.add(Instrument.USDTRY);
        }

        if (config.has("USDZAR"))
        {
            System.out.println("Subscribing USDZAR");
            instruments.add(Instrument.USDZAR);
        }

        if (config.has("USA500.IDXUSD"))
        {
            System.out.println("Subscribing USA500.IDXUSD");
            instruments.add(Instrument.fromString("USA500.IDX/USD"));
        }

        if (config.has("EUS.IDXEUR"))
        {
            System.out.println("Subscribing EUS.IDXEUR");
            instruments.add(Instrument.fromString("EUS.IDX/EUR"));
        }

        if (config.has("BRENT.CMDUSD"))
        {
            System.out.println("Subscribing BRENT.CMDUSD");
            instruments.add(Instrument.fromString("BRENT.CMD/USD"));
        }

        if (config.has("LIGHT.CMDUSD"))
        {
            System.out.println("Subscribing LIGHT.CMDUSD");
            instruments.add(Instrument.fromString("LIGHT.CMD/USD"));
        }

        if (config.has("COPPER.CMDUSD"))
        {
            System.out.println("Subscribing COPPER.CMDUSD");
            instruments.add(Instrument.fromString("COPPER.CMD/USD"));
        }

        if (config.has("XAUUSD"))
        {
            System.out.println("Subscribing XAUUSD");
            instruments.add(Instrument.fromString("XAU/USD"));
        }

        if (config.has("XAGUSD"))
        {
            System.out.println("Subscribing XAGUSD");
            instruments.add(Instrument.fromString("XAG/USD"));
        }

        System.out.println("Subscribing instruments to Dukascopy servers...");
        client.setSubscribedInstruments(instruments);
    }
}
