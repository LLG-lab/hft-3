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

            String model = config.getJSONObject(instr).getString("investition_policy_model");

            if (model.equals("const_units"))
            {
                return (config.getJSONObject(instr).getDouble("amount")) / 1000.0;
            }
            else if (model.equals("equity_percentage"))
            {
                double funct = config.getJSONObject(instr).getDouble("function");
                double ret = (funct * this.account.getEquity()) / 1000000.0;

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
                }
                if (config.has("EURUSD"))
                {
                    instrument_enum_str += ";EUR/USD";
                }
                if (config.has("GBPUSD"))
                {
                    instrument_enum_str += ";GBP/USD";
                }
                if (config.has("NZDUSD"))
                {
                    instrument_enum_str += ";NZD/USD";
                }
                if (config.has("USDCAD"))
                {
                    instrument_enum_str += ";USD/CAD";
                }
                if (config.has("USDCHF"))
                {
                    instrument_enum_str += ";USD/CHF";
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
            slPrice = tick.getAsk() - this.getSlPips(instrument) * instrument.getPipValue();
            orderCmd = OrderCommand.BUY;
        }
        else
        {
            slPrice = tick.getBid() + this.getSlPips(instrument) * instrument.getPipValue();
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
        ///// Może niepotrzebne - na razie dla testu zostawiamy
        //if (! instrument.equals(Instrument.EURUSD)) {
        //    System.out.println("onAccount notify got called for instrument != EURUSD.");
        //    return;
        //}
        ///// koniec testu. ////

        try
        {
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
