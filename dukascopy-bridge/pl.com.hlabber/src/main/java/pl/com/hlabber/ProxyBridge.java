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

import java.io.*;
import java.net.*;
import java.util.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

import org.json.*;

import com.dukascopy.api.IEngine.OrderCommand;
import com.dukascopy.api.*;

public class ProxyBridge implements IStrategy
{
    private IEngine engine   = null;
    private IContext context = null;
    private IAccount account = null;

    private Socket clientSocket;
    private DataOutputStream outToServer;
    private BufferedReader inFromServer;

    private static final DateTimeFormatter dtf = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.000");

    private JSONObject config;

    private Set<Instrument> instruments;
    private int chkSubscribeCounter;

    //
    // External configurable parameters.
    //

    private int connectionPort;

    private static String getDateTimeStr()
    {
        LocalDateTime now = LocalDateTime.now();
        return dtf.format(now);
        //return "2019-10-26 20:45:31.000";
    }

    private String instrument2instrumentstr(Instrument instrument)
    {
        String ret = "";

        if (instrument.equals(Instrument.AUDUSD))
        {
            ret = "AUDUSD";
        }
        else if (instrument.equals(Instrument.EURUSD))
        {
            ret = "EURUSD";
        }
        else if (instrument.equals(Instrument.GBPUSD))
        {
            ret = "GBPUSD";
        }
        else if (instrument.equals(Instrument.NZDUSD))
        {
            ret = "NZDUSD";
        }
        else if (instrument.equals(Instrument.USDCAD))
        {
            ret = "USDCAD";
        }
        else if (instrument.equals(Instrument.USDCHF))
        {
            ret = "USDCHF";
        }
        else if (instrument.equals(Instrument.USDJPY))
        {
            ret = "USDJPY";
        }
        else if (instrument.equals(Instrument.USDCNH))
        {
            ret = "USDCNH";
        }
        else if (instrument.equals(Instrument.USDDKK))
        {
            ret = "USDDKK";
        }
        else if (instrument.equals(Instrument.USDHKD))
        {
            ret = "USDHKD";
        }
        else if (instrument.equals(Instrument.USDHUF))
        {
            ret = "USDHUF";
        }
        else if (instrument.equals(Instrument.USDMXN))
        {
            ret = "USDMXN";
        }
        else if (instrument.equals(Instrument.USDNOK))
        {
            ret = "USDNOK";
        }
        else if (instrument.equals(Instrument.USDPLN))
        {
            ret = "USDPLN";
        }
        else if (instrument.equals(Instrument.USDRUB))
        {
            ret = "USDRUB";
        }
        else if (instrument.equals(Instrument.USDSEK))
        {
            ret = "USDSEK";
        }
        else if (instrument.equals(Instrument.USDSGD))
        {
            ret = "USDSGD";
        }
        else if (instrument.equals(Instrument.USDTRY))
        {
            ret = "USDTRY";
        }
        else if (instrument.equals(Instrument.USDZAR))
        {
            ret = "USDZAR";
        }

        return ret;
    }

    private int getSlPips(Instrument instrument)
    {
        try
        {
            String instr = instrument2instrumentstr(instrument);

            return config.getJSONObject(instr).getInt("hard_stoploss");
        }
        catch (JSONException e)
        {
            System.err.println("Configuration ERROR: " + e.getMessage());
        }

        return 0;
    }

    private double getAmount(Instrument instrument)
    {
        try
        {
            String instr = instrument2instrumentstr(instrument);

            String model = config.getJSONObject(instr).getJSONObject("investition_policy").getString("model");

            if (model.equals("const_units"))
            {
                return (config.getJSONObject(instr).getJSONObject("investition_policy").getDouble("amount")) / 1000000.0;
            }
            else if (model.equals("equity_percentage"))
            {
                double funct = config.getJSONObject(instr).getJSONObject("investition_policy").getDouble("function");                double ret = (funct * this.account.getEquity()) / 1000000.0;

                return ((ret < 0.001) ?  0.001 : ret);
            }
            else
            {
                System.err.println("Configuration ERROR: Illegal model ["
                                       + model + "].");
            }
        }
        catch (JSONException e)
        {
            System.err.println("Configuration ERROR: " + e.getMessage());
        }

        return 0.0;
    }

    public ProxyBridge(int hft_listening_port, JSONObject cfg)
    {
        config = cfg;
        connectionPort = hft_listening_port;
        int connect_attempt = 0;
        this.instruments = new HashSet<>();
        this.chkSubscribeCounter = 0;

        while (true)
        {
            try
            {
                this.clientSocket = new Socket("localhost", this.connectionPort);
                this.outToServer = new DataOutputStream(clientSocket.getOutputStream());
                this.inFromServer = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));

                String instrument_enum_str = "";

                if (config.has("AUDUSD"))
                {
                    instrument_enum_str += ";AUD/USD";
                    instruments.add(Instrument.AUDUSD);
                }
                if (config.has("EURUSD"))
                {
                    instrument_enum_str += ";EUR/USD";
                    instruments.add(Instrument.EURUSD);
                }
                if (config.has("GBPUSD"))
                {
                    instrument_enum_str += ";GBP/USD";
                    instruments.add(Instrument.GBPUSD);
                }
                if (config.has("NZDUSD"))
                {
                    instrument_enum_str += ";NZD/USD";
                    instruments.add(Instrument.NZDUSD);
                }
                if (config.has("USDCAD"))
                {
                    instrument_enum_str += ";USD/CAD";
                    instruments.add(Instrument.USDCAD);
                }
                if (config.has("USDCHF"))
                {
                    instrument_enum_str += ";USD/CHF";
                    instruments.add(Instrument.USDCHF);
                }
                if (config.has("USDJPY"))
                {
                    instrument_enum_str += ";USD/JPY";
                    instruments.add(Instrument.USDJPY);
                }
                if (config.has("USDCNH"))
                {
                    instrument_enum_str += ";USD/CNH";
                    instruments.add(Instrument.USDCNH);
                }
                if (config.has("USDDKK"))
                {
                    instrument_enum_str += ";USD/DKK";
                    instruments.add(Instrument.USDDKK);
                }
                if (config.has("USDHKD"))
                {
                    instrument_enum_str += ";USD/HKD";
                    instruments.add(Instrument.USDHKD);
                }
                if (config.has("USDHUF"))
                {
                    instrument_enum_str += ";USD/HUF";
                    instruments.add(Instrument.USDHUF);
                }
                if (config.has("USDMXN"))
                {
                    instrument_enum_str += ";USD/MXN";
                    instruments.add(Instrument.USDMXN);
                }
                if (config.has("USDNOK"))
                {
                    instrument_enum_str += ";USD/NOK";
                    instruments.add(Instrument.USDNOK);
                }
                if (config.has("USDPLN"))
                {
                    instrument_enum_str += ";USD/PLN";
                    instruments.add(Instrument.USDPLN);
                }
                if (config.has("USDRUB"))
                {
                    instrument_enum_str += ";USD/RUB";
                    instruments.add(Instrument.USDRUB);
                }
                if (config.has("USDSEK"))
                {
                    instrument_enum_str += ";USD/SEK";
                    instruments.add(Instrument.USDSEK);
                }
                if (config.has("USDSGD"))
                {
                    instrument_enum_str += ";USD/SGD";
                    instruments.add(Instrument.USDSGD);
                }
                if (config.has("USDTRY"))
                {
                    instrument_enum_str += ";USD/TRY";
                    instruments.add(Instrument.USDTRY);
                }
                if (config.has("USDZAR"))
                {
                    instrument_enum_str += ";USD/ZAR";
                    instruments.add(Instrument.USDZAR);
                }

                outToServer.writeBytes("SUBSCRIBE"+instrument_enum_str+"\n");
                String reply = inFromServer.readLine();

                if (! reply.equals("OK")) {
                    System.err.println("HFT server respond fail reply: [" + reply +']');
                    System.exit(1);
                }

                break;
            }
            catch (IOException e)
            {
                System.err.println("Socket error: [" + e.getMessage() + ']');
            }

            if (connect_attempt++ == 7)
            {
                System.err.println("Failed to connect to HFT server. Giving up.");
                System.exit(1);
            }

            try
            {
                Thread.sleep(1000);
            }
            catch (InterruptedException e)
            {
                System.err.println(e.getMessage());
            }
        }
    }

    private void checkDukascopySubscribed()
    {
        if (! this.context.getSubscribedInstruments().containsAll(this.instruments)
               && this.chkSubscribeCounter <= 0)
        {
            System.out.println("WARNING! Some instruments still unsubscribed");

            this.context.setSubscribedInstruments(this.instruments);
            this.chkSubscribeCounter = 50;
        }

        this.chkSubscribeCounter--;
    }

    private void closeOrders(List<IOrder> orders) throws JFException
    {
        for(IOrder o: orders)
        {
            if(o.getState() == IOrder.State.FILLED || o.getState() == IOrder.State.OPENED)
            {
                o.close();
            }
        }
    }

    public void onStart(IContext context) throws JFException
    {
        System.out.println("Starting Proxy Bridge: onStart notify got called.");

        this.engine = context.getEngine();
        this.context = context;
    }

    public void onStop() throws JFException
    {
        System.out.println("onStop notify got called.");

        //
        // Close all orders when automate stops.
        //

        for (IOrder order : engine.getOrders())
        {
            order.close();
        }

        try
        {
            this.clientSocket.close();
        } catch (IOException e) {
            System.err.println("Failed to close connection to hft server: [" + e.getMessage() + ']');
        }
    }

    private void submitOrder(Instrument instrument, ITick tick, boolean isLong) throws JFException
    {
        double slPrice;
        OrderCommand orderCmd;

        if (isLong)
        {
            if (this.getSlPips(instrument) == 0)
            {
                slPrice = 0;
            }
            else
            {
                slPrice = tick.getAsk() - this.getSlPips(instrument) * instrument.getPipValue();
            }

            orderCmd = OrderCommand.BUY;
        }
        else
        {
            if (this.getSlPips(instrument) == 0)
            {
                slPrice = 0;
            }
            else
            {
                slPrice = tick.getBid() + this.getSlPips(instrument) * instrument.getPipValue();
            }

            orderCmd = OrderCommand.SELL;
        }

        //
        // Close previous orders.
        //

        for (IOrder o : engine.getOrders())
        {
            if ((o.getState() == IOrder.State.FILLED || o.getState() == IOrder.State.OPENED) &&
                    instrument.equals(o.getInstrument()))
            {
                o.close();
            }
        }

        engine.submitOrder("hft_" + orderCmd.toString() + System.currentTimeMillis(),
                               instrument, orderCmd, this.getAmount(instrument), 0, 2, slPrice, 0);
    }

    public void onTick(Instrument instrument, ITick tick) throws JFException
    {
        try
        {
            this.checkDukascopySubscribed();

            // Format bedzie taki:
            // instrument;ASK;ile_siana;pozycja
            // account.getCreditLine(), account.getBalance()

            outToServer.writeBytes("TICK;"+instrument+";"+getDateTimeStr()+";"+tick.getAsk()+";"+this.account.getBalance()+";credit_line="+this.account.getCreditLine() +'\n');
            String reply = inFromServer.readLine();

            if (reply.equals("OK")) {
                return;
            } else if (reply.equals("CLOSE")) {
                //closeOrders(engine.getOrders());
                for (IOrder order : engine.getOrders())
                {
                    if (instrument.equals(order.getInstrument()))
                    {
                        order.close();
                    }
                }
            } else if (reply.equals("LONG")) {
                submitOrder(instrument, tick, true);
            } else if (reply.equals("SHORT")) {
                submitOrder(instrument, tick, false);
            } else {
                System.err.println("Received unrecognized operation from HFT server: ["+reply+']');
            }
        }
        catch (IOException e)
        {
            System.err.println("HFT server I/O error: ["+e.getMessage()+']');
            System.exit(1);
        }
    }

    public void onBar(Instrument instrument, Period period, IBar askBar, IBar bidBar)
    {
        /* Nothing to do */
    }

    public void onMessage(IMessage message) throws JFException
    {
        /* Nothing to do */
    }

    public void onAccount(IAccount acc) throws JFException
    {
        System.out.println("onAccount notify got called.");

        this.account = acc;
    }
}
