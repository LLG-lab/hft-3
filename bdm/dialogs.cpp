/**********************************************************************\
**                                                                    **
**                 -=≡≣ BitMex Data Manager  ≣≡=-                    **
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

#include <dialogs.hpp>
#include <file_operations.hpp>

#include <map>
#include <vector>
#include <algorithm>
#include <newt.h>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/algorithm/string.hpp>

namespace bdm {
namespace dialogs {

static std::string timestamp2str(int timestamp)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    boost::posix_time::ptime t(
         date(1970, Jan, 1),  // The epoch.
         boost::posix_time::seconds(timestamp) + // Number of seconds.
         boost::posix_time::microseconds(0)  // And the micros too.
    );

    return to_simple_string(t) + std::string(" UTC");
}

void announcement(const std::string &title, const std::string &msg)
{
    newtComponent form, text, button;

    char *message = (char *) malloc(msg.length() + 1);
    strcpy(message, msg.c_str());

    text = newtTextboxReflowed(1, 1, message, 50, 5, 5, 0);
    button = newtButton(25, newtTextboxGetNumLines(text) + 3, "Ok");

    newtCenteredWindow(57, newtTextboxGetNumLines(text) + 7, title.c_str());

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, text, button, NULL);

    newtRunForm(form);
    newtFormDestroy(form);
    newtPopWindow();

    free(message);
}

bool ask_question(const std::string &msg)
{
    bool ret;
    newtComponent form, text, yes_button, no_button;

    char *message = (char *) malloc(msg.length() + 1);
    strcpy(message, msg.c_str());

    text = newtTextboxReflowed(1, 1, message, 50, 5, 5, 0);
    no_button  = newtButton(12, newtTextboxGetNumLines(text) + 3, "No");
    yes_button = newtButton(57-16, newtTextboxGetNumLines(text) + 3, "Yes");

    newtCenteredWindow(57, newtTextboxGetNumLines(text) + 7, "Question");

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, text, no_button, yes_button, NULL);

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT)
        {
            if (es.u.co == no_button)
            {
                ret = false;
                break;
            }
            else if (es.u.co == yes_button)
            {
                ret = true;
                break;
            }
        }
    }

    newtFormDestroy(form);
    newtPopWindow();

    free(message);

    return ret;
}

static void scan_dir(std::map<int, std::string> &out)
{
    using namespace boost::filesystem;

    out.clear();

    int index = 0;

    out[index++] = "../";

    std::vector<std::string> dirs;
    std::vector<std::string> files;

    for(auto &entry : boost::make_iterator_range(directory_iterator(current_path()), {}))
    {
        if (is_directory(entry.path()))
        {
            dirs.push_back(entry.path().filename().string() + std::string("/"));
        }
    }

    for(auto &entry : boost::make_iterator_range(directory_iterator(current_path()), {}))
    {
        if (! is_directory(entry.path()))
        {
            files.push_back(entry.path().filename().string());
        }
    }

    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    for (auto &entry : dirs)
    {
        out[index++] = entry;
    }

    for (auto &entry : files)
    {
        out[index++] = entry;
    }
}

typedef struct _save_as_file_callback_private
{
    _save_as_file_callback_private(const std::map<int, std::string> &items_data, newtComponent select_label_data, newtComponent file_name_entry_data)
        : select_label(select_label_data), items(items_data), file_name_entry(file_name_entry_data) {}
    _save_as_file_callback_private(void) = delete;

    newtComponent select_label;
    newtComponent file_name_entry;
    const std::map<int, std::string> &items;
    int current_selection_index;

} save_as_file_callback_private;

static void save_as_file_callback(newtComponent co, void *dev_private)
{
    using namespace boost::filesystem;

    auto now_path = current_path().string();

    save_as_file_callback_private *lcp = (save_as_file_callback_private *) dev_private;
    lcp -> current_selection_index = (long int)(newtListboxGetCurrent(co));
    std::string selected = now_path + std::string("/") + lcp -> items.at(lcp -> current_selection_index);

    if (selected.length() > 40)
    {
        selected = std::string("(...)") + selected.substr(selected.length() - 40 + 5);
    }

    newtLabelSetText(lcp -> select_label, "                                          ");
    newtLabelSetText(lcp -> select_label, selected.c_str());
    newtRefresh();
}

std::string save_as_file(void)
{
    using namespace boost::filesystem;

    auto original_path = current_path();
    std::string ret = "";
    std::map<int, std::string> dir_items;

    newtCenteredWindow(45, 25, "Save as file");

    newtComponent filelist_listbox = newtListbox(3, 2, 16, NEWT_FLAG_RETURNEXIT | NEWT_FLAG_BORDER);
    newtComponent selected_file_label = newtLabel(3, 1, "");
    newtComponent file_name_entry = newtEntry(3, 19, "network.json", 40, NULL, NEWT_FLAG_RETURNEXIT |  NEWT_ENTRY_SCROLL);

    newtComponent ok_button = newtButton(10, 21, "Save");
    newtComponent cancel_button = newtButton(22, 21, "Cancel");
    newtComponent form = newtForm(NULL, NULL, 0);

    newtListboxSetWidth(filelist_listbox, 40);

    save_as_file_callback_private callback_info(dir_items, selected_file_label, file_name_entry);
    newtComponentAddCallback(filelist_listbox, save_as_file_callback, &callback_info);

    newtFormAddComponents(form,
                          filelist_listbox,
                          selected_file_label,
                          file_name_entry,
                          ok_button,
                          cancel_button,
                          NULL);

    scan_dir(dir_items);
    for (auto &item : dir_items)
    {
        newtListboxAppendEntry(filelist_listbox, item.second.c_str(), (void *)((long int)(item.first)));
    }

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT)
        {
            if (es.u.co == cancel_button)
            {
                break;
            }
            else if (es.u.co == filelist_listbox)
            {
                std::string now_path = current_path().string() + std::string("/") + dir_items[callback_info.current_selection_index];

                if (is_directory(path(now_path)))
                {
                    current_path(now_path);
                    newtListboxClear(filelist_listbox);
                    scan_dir(dir_items);
                    for (auto &item : dir_items)
                    {
                        newtListboxAppendEntry(filelist_listbox, item.second.c_str(), (void *)((long int)(item.first)));
                    }
                    callback_info.current_selection_index = 0;
                }
                else
                {
                    // Ustawienie dir_items[callback_info.current_selection_index]
                    // w polu file_name_entry.
                    newtEntrySet(file_name_entry, dir_items[callback_info.current_selection_index].c_str(), 1);
                }
            }
            else if (es.u.co == ok_button || es.u.co == file_name_entry)
            {
                std::string selected_file = boost::trim_copy(std::string(newtEntryGetValue(file_name_entry)));
                std::string selected_path = current_path().string()
                                            + std::string("/") + selected_file;

                // 1. Sprawdzenie, czy selected_file jest ciągiem pustym,
                //    jesli tak, to wyswietlic komunikat o błedzie,
                //    kontunowanie pętli.
                // 2. Sprawdzenie, czy selected_path jest katalogiem,
                //    jesli tak, to wyświetlic komunikat o błędzie,
                //    kontynuowanie pętli.
                // 3. Sprawdzenie, czy selected_path jest plikiem
                //    jesli tak, to zapytać usera czy sie zgodzi nadpisać,
                //    jesli sie zgodzi to zwracamy selected_path, jesli
                //    sie nie zgodzi, kontynuowanie pętli.
                // 4. Zwracamy selected_path.

                boost::system::error_code ec;

                if (selected_file.length() == 0)
                {
                    announcement("ERROR", "You must specify file name");
                }
                else if (boost::filesystem::is_directory(boost::filesystem::path(selected_path), ec))
                {
                    announcement("ERROR", "You specified directory instead of file name");
                }
                else if (boost::filesystem::is_symlink(boost::filesystem::path(selected_path), ec))
                {
                    announcement("ERROR", "Specified target cannot be existing symlink");
                }
                else if (boost::filesystem::is_regular_file(boost::filesystem::path(selected_path), ec))
                {
                    std::string question = std::string("File `") + selected_file
                                           + std::string("' already exist. Overwrite?");

                    if (ask_question(question))
                    {
                        ret = selected_path;
                        break;
                    }
                }
                else
                {
                    ret = selected_path;
                    break;
                }
            }
        }
    }

    /* End of dialog */
    newtFormDestroy(form);
    newtPopWindow();

    current_path(original_path);

    return ret;
}

typedef struct _file_browse_callback_private
{
    _file_browse_callback_private(const std::map<int, std::string> &items_data, newtComponent select_label_data)
        : select_label(select_label_data), items(items_data) {}
    _file_browse_callback_private(void) = delete;

    newtComponent select_label;
    const std::map<int, std::string> &items;
    int current_selection_index;

} file_browse_callback_private;

static void file_browse_callback(newtComponent co, void *dev_private)
{
    using namespace boost::filesystem;

    auto now_path = current_path().string();

    file_browse_callback_private *lcp = (file_browse_callback_private *) dev_private;
    lcp -> current_selection_index = (long int)(newtListboxGetCurrent(co));
    std::string selected = now_path + std::string("/") + lcp -> items.at(lcp -> current_selection_index);

    if (selected.length() > 40)
    {
        selected = std::string("(...)") + selected.substr(selected.length() - 40 + 5);
    }

    newtLabelSetText(lcp -> select_label, "                                          ");
    newtLabelSetText(lcp -> select_label, selected.c_str());
    newtRefresh();
}

std::string file_browse(void)
{
    using namespace boost::filesystem;

    auto original_path = current_path();
    std::string ret = "";
    std::map<int, std::string> dir_items;

    newtCenteredWindow(45, 25, "Browse file");

    newtComponent filelist_listbox = newtListbox(3, 2, 18, NEWT_FLAG_RETURNEXIT | NEWT_FLAG_BORDER);
    newtComponent selected_file_label = newtLabel(3, 1, "");

    newtComponent ok_button = newtButton(10, 21, "Ok");
    newtComponent cancel_button = newtButton(22, 21, "Cancel");
    newtComponent form = newtForm(NULL, NULL, 0);

    newtListboxSetWidth(filelist_listbox, 40);

    file_browse_callback_private callback_info(dir_items, selected_file_label);
    newtComponentAddCallback(filelist_listbox, file_browse_callback, &callback_info);

    newtFormAddComponents(form,
                          filelist_listbox,
                          selected_file_label,
                          ok_button,
                          cancel_button,
                          NULL);

    scan_dir(dir_items);
    for (auto &item : dir_items)
    {
        newtListboxAppendEntry(filelist_listbox, item.second.c_str(), (void *)((long int)(item.first)));
    }

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT)
        {
            if (es.u.co == cancel_button)
            {
                break;
            }
            else if (es.u.co == ok_button || es.u.co == filelist_listbox)
            {
                std::string now_path = current_path().string() + std::string("/") + dir_items[callback_info.current_selection_index];

                if (is_directory(path(now_path)))
                {
                    current_path(now_path);
                    newtListboxClear(filelist_listbox);
                    scan_dir(dir_items);
                    for (auto &item : dir_items)
                    {
                        newtListboxAppendEntry(filelist_listbox, item.second.c_str(), (void *)((long int)(item.first)));
                    }
                    callback_info.current_selection_index = 0;
                }
                else
                {
                    ret = now_path;
                    break;
                }
            }
        }
    }

    newtFormDestroy(form);
    newtPopWindow();

    current_path(original_path);

    return ret;
}

bool settings(appconfig &cfg)
{
    bool ret = false;

    newtCenteredWindow(40, 21, "Settings");
/*
    newtComponent prod_label1 = newtLabel(3, 1, "Bitmex PROD credentials");
    newtComponent prod_label2 = newtLabel(8, 2, "e-mail");
    newtComponent prod_email_entry = newtEntry(16, 2, NULL, 20, NULL, NEWT_ENTRY_SCROLL);
    newtComponent prod_label3 = newtLabel(6, 3, "password");
    newtComponent prod_pass_entry = newtEntry(16, 3, NULL, 20, NULL, NEWT_ENTRY_SCROLL | NEWT_FLAG_PASSWORD);

    newtComponent testnet_label1 = newtLabel(3, 4, "Bitmex TESTNET credentials");
    newtComponent testnet_label2 = newtLabel(8, 5, "e-mail");
    newtComponent testnet_email_entry = newtEntry(16, 5, NULL, 20, NULL, NEWT_ENTRY_SCROLL);
    newtComponent testnet_label3 = newtLabel(6, 6, "password");
    newtComponent testnet_pass_entry = newtEntry(16, 6, NULL, 20, NULL, NEWT_ENTRY_SCROLL | NEWT_FLAG_PASSWORD);
*/
    newtComponent hft_prog_path_label = newtLabel(3, 1, "HFT application path");
    newtComponent hft_prog_path_entry = newtEntry(3, 2, NULL, 20, NULL, NEWT_ENTRY_SCROLL);
    newtComponent hft_prog_browse_btn = newtCompactButton(24, 2, "Browse...");

    newtComponent records_per_hftr_label      = newtLabel(7, 4, "Records per hftr");
    newtComponent records_per_hftr_entry      = newtEntry(25, 4, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent network_arch_label          = newtLabel(3, 5, "Network architecture");
    newtComponent network_arch_entry          = newtEntry(25, 5, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent train_goal_label            = newtLabel(11, 6, "Train goal ≤");
    newtComponent train_goal_entry            = newtEntry(25, 6, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent learn_coefficient_label     = newtLabel(6, 7, "Learn coefficient");
    newtComponent learn_coefficient_entry     = newtEntry(25, 7, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent trade_setup_target_label    = newtLabel(5, 8, "Trade setup target");
    newtComponent trade_setup_target_entry    = newtEntry(25, 8, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent market_collector_size_label = newtLabel(2, 9, "Market collector size");
    newtComponent market_collector_size_entry = newtEntry(25, 9, NULL, 11, NULL, NEWT_FLAG_WRAP);
    newtComponent hftr_offset_label           = newtLabel(1, 10,"HFTR generation offset");
    newtComponent hftr_offset_entry           = newtEntry(25, 10, NULL, 11, NULL, NEWT_FLAG_WRAP);

    newtComponent ok_button = newtButton(10, 17, "Ok");
    newtComponent cancel_button = newtButton(22, 17, "Cancel");
    newtComponent form = newtForm(NULL, NULL, 0);

/*    newtEntrySet(prod_email_entry,        cfg.bitmex_prod_email.c_str(), 0);
    newtEntrySet(prod_pass_entry,         cfg.bitmex_prod_password.c_str(), 0);
    newtEntrySet(testnet_email_entry,     cfg.bitmex_testnet_email.c_str(), 0);
    newtEntrySet(testnet_pass_entry,      cfg.bitmex_testnet_password.c_str(), 0); */
    newtEntrySet(hft_prog_path_entry,         cfg.hft_program_path.c_str(), 0);
    newtEntrySet(records_per_hftr_entry,      std::to_string(cfg.records_per_hftr).c_str(), 0);
    newtEntrySet(network_arch_entry,          cfg.network_arch.c_str(), 0);
    newtEntrySet(train_goal_entry,            std::to_string(cfg.train_goal).c_str(), 0);
    newtEntrySet(learn_coefficient_entry,     std::to_string(cfg.learn_coeeficient).c_str(), 0);
    newtEntrySet(trade_setup_target_entry,    std::to_string(cfg.trade_setup_target).c_str(), 0);
    newtEntrySet(market_collector_size_entry, std::to_string(cfg.market_collector_size).c_str(), 0);
    newtEntrySet(hftr_offset_entry,           std::to_string(cfg.hftr_offset).c_str(), 0);

    newtFormAddComponents(form,
                          hft_prog_path_label,
                          hft_prog_path_entry,
                          hft_prog_browse_btn,
                          records_per_hftr_label,
                          records_per_hftr_entry,
                          network_arch_label,
                          network_arch_entry,
                          train_goal_label,
                          train_goal_entry,
                          learn_coefficient_label,
                          learn_coefficient_entry,
                          trade_setup_target_label,
                          trade_setup_target_entry,
                          market_collector_size_label,
                          market_collector_size_entry,
                          hftr_offset_label,
                          hftr_offset_entry,
                          ok_button, cancel_button, NULL);

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT)
        {
            if (es.u.co == cancel_button)
            {
                ret = false;

                break;
            }
            else if (es.u.co == ok_button)
            {
                cfg.hft_program_path = newtEntryGetValue(hft_prog_path_entry);
                cfg.records_per_hftr = std::atoi(newtEntryGetValue(records_per_hftr_entry));
                cfg.network_arch = newtEntryGetValue(network_arch_entry);
                cfg.train_goal = std::atof(newtEntryGetValue(train_goal_entry));
                cfg.learn_coeeficient = std::atof(newtEntryGetValue(learn_coefficient_entry));
                cfg.trade_setup_target = std::atoi(newtEntryGetValue(trade_setup_target_entry));
                cfg.market_collector_size = std::atoi(newtEntryGetValue(market_collector_size_entry));
                cfg.hftr_offset = std::atoi(newtEntryGetValue(hftr_offset_entry));

                ret = true;
                break;
            }
            else if (es.u.co == hft_prog_browse_btn)
            {
                std::string hft_file_name = file_browse();

                if (hft_file_name.length() > 0)
                {
                    newtEntrySet(hft_prog_path_entry, hft_file_name.c_str(), 1);
                }
            }
        }
    }

    newtFormDestroy(form);
    newtPopWindow();

    return ret;
}

typedef struct _main_menu_selection_callback_private
{
    _main_menu_selection_callback_private(newtComponent l1, newtComponent l2, newtComponent l3, newtComponent l4)
        : label1(l1), label2(l2), label3(l3), label4(l4) {}
    _main_menu_selection_callback_private(void) = delete;

    newtComponent label1;
    newtComponent label2;
    newtComponent label3;
    newtComponent label4;

} main_menu_selection_callback_private;

static void main_menu_selection_callback(newtComponent co, void *dev_private)
{
    main_menu_selection_callback_private *mmscp = (main_menu_selection_callback_private *) dev_private;

    long int current_selection_index = (long int)(newtListboxGetCurrent(co));

    switch (current_selection_index)
    {
        case MMS_GENERATE_PROD_NETWORK:
             newtLabelSetText(mmscp -> label1, "Up to date historical     ");
             newtLabelSetText(mmscp -> label2, "production candles data,  ");
             newtLabelSetText(mmscp -> label3, "then generate and train   ");
             newtLabelSetText(mmscp -> label4, "production neural network.");
             break;
        case MMS_GENERATE_TESTNET_NETWORK:
             newtLabelSetText(mmscp -> label1, "Up to date historical     ");
             newtLabelSetText(mmscp -> label2, "testnet candles data,     ");
             newtLabelSetText(mmscp -> label3, "then generate and train   ");
             newtLabelSetText(mmscp -> label4, "testnet neural network.   ");
             break;
        case MMS_BROWSE_PROD_NETWORKS:
             newtLabelSetText(mmscp -> label1, "Browse previously created ");
             newtLabelSetText(mmscp -> label2, "neural networks for       ");
             newtLabelSetText(mmscp -> label3, "BitMex production account.");
             newtLabelSetText(mmscp -> label4, "                          ");
             break;
        case MMS_BROWSE_TESTNET_NETWORKS:
             newtLabelSetText(mmscp -> label1, "Browse previously created ");
             newtLabelSetText(mmscp -> label2, "neural networks for       ");
             newtLabelSetText(mmscp -> label3, "BitMex testnet account.   ");
             newtLabelSetText(mmscp -> label4, "                          ");
             break;
        case MMS_SETTINGS:
             newtLabelSetText(mmscp -> label1, "BitMex accounts credential");
             newtLabelSetText(mmscp -> label2, "setup and neural networks ");
             newtLabelSetText(mmscp -> label3, "parameters setting.       ");
             newtLabelSetText(mmscp -> label4, "                          ");
             break;
        case MMS_EXIT:
             newtLabelSetText(mmscp -> label1, "Exit program.             ");
             newtLabelSetText(mmscp -> label2, "                          ");
             newtLabelSetText(mmscp -> label3, "                          ");
             newtLabelSetText(mmscp -> label4, "                          ");
             break;
    }
}

main_menu_selections main_menu(void)
{
    main_menu_selections ret = MMS_EXIT;

    newtCenteredWindow(60, 10, "Main menu");

    newtComponent selection_listbox = newtListbox(0, 0, 10, NEWT_FLAG_RETURNEXIT | NEWT_FLAG_BORDER);
    newtComponent description_label1 = newtLabel(33, 2, "Up to date historical");
    newtComponent description_label2 = newtLabel(33, 3, "production candles data,");
    newtComponent description_label3 = newtLabel(33, 4, "then generate and train");
    newtComponent description_label4 = newtLabel(33, 5, "production neural network.");

    newtListboxSetWidth(selection_listbox, 31);
    main_menu_selection_callback_private dev_private(description_label1,
                                                     description_label2,
                                                     description_label3,
                                                     description_label4);

    newtListboxAppendEntry(selection_listbox, "Generate PROD network", (void *)(MMS_GENERATE_PROD_NETWORK));
    newtListboxAppendEntry(selection_listbox, "Generate TESTNET network", (void *)(MMS_GENERATE_TESTNET_NETWORK));
    newtListboxAppendEntry(selection_listbox, "Browse PROD networks...", (void *)(MMS_BROWSE_PROD_NETWORKS));
    newtListboxAppendEntry(selection_listbox, "Browse TESTNET networks...", (void *)(MMS_BROWSE_TESTNET_NETWORKS));
    newtListboxAppendEntry(selection_listbox, "Settings..", (void *)(MMS_SETTINGS));
    newtListboxAppendEntry(selection_listbox, "--EXIT--", (void *)(MMS_EXIT));

    newtComponent form = newtForm(NULL, NULL, 0);

    newtComponentAddCallback(selection_listbox, main_menu_selection_callback, &dev_private);

    newtFormAddComponents(form,
                          selection_listbox,
                          description_label1,
                          description_label2,
                          description_label3,
                          description_label4,
                          NULL);

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT && es.u.co == selection_listbox)
        {
            long int current_selection_index = (long int)(newtListboxGetCurrent(selection_listbox));

            newtFormDestroy(form);
            newtPopWindow();

            return (main_menu_selections) (current_selection_index);
        }
    }
}

typedef struct _browse_networks_selection_callback_private
{
    _browse_networks_selection_callback_private(db::networks_container &d, newtComponent sdl, newtComponent edl, newtComponent al)
        : data(d),
          start_date_label(sdl),
          end_date_label(edl),
          architecture_label(al) {}

    _browse_networks_selection_callback_private(void) = delete;

    db::networks_container &data;
    newtComponent start_date_label;
    newtComponent end_date_label;
    newtComponent architecture_label;

} browse_networks_selection_callback_private;

static void browse_networks_selection_callback(newtComponent co, void *dev_private)
{
    browse_networks_selection_callback_private *bnscp = (browse_networks_selection_callback_private *) dev_private;

    long int current_selection_index = (long int)(newtListboxGetCurrent(co));

    std::string start_date_text   = std::string("Start: ") + timestamp2str(bnscp -> data.at(current_selection_index).start_time);
    std::string end_date_text     = std::string("  End: ") + timestamp2str(bnscp -> data.at(current_selection_index).ent_time);
    std::string architecture_text = std::string("Architecture: ") + bnscp -> data.at(current_selection_index).architecture;

    newtLabelSetText(bnscp -> start_date_label, start_date_text.c_str());
    newtLabelSetText(bnscp -> end_date_label, end_date_text.c_str());
    newtLabelSetText(bnscp -> architecture_label, "                               ");
    newtLabelSetText(bnscp -> architecture_label, architecture_text.c_str());
}

void browse_networks(mode_operation mode, db::networks_container &data)
{
    std::string title = "Browse";

    switch (mode)
    {
        case mode_operation::PRODUCTION:
             title += " Production networks";
             break;
        case mode_operation::TESTNET:
             title += " Testnet networks";
             break;
        default:
             throw std::runtime_error("browse_networks: Illegal mode operation");
    }

    newtCenteredWindow(66, 18, title.c_str());

    std::string start_date_text   = std::string("Start: ") + timestamp2str(data.at(0).start_time);
    std::string end_date_text     = std::string("  End: ") + timestamp2str(data.at(0).ent_time);
    std::string architecture_text = std::string("Architecture: ") + data.at(0).architecture;

    newtComponent available_networks_label = newtLabel(1, 1, "Available networks:");
    newtComponent selection_listbox = newtListbox(1, 2, 16, NEWT_FLAG_BORDER | NEWT_FLAG_SCROLL);
    newtComponent start_date_label  = newtLabel(34, 2, start_date_text.c_str());
    newtComponent end_date_label    = newtLabel(34, 3, end_date_text.c_str());
    newtComponent architecture_label= newtLabel(34, 4, architecture_text.c_str());
    newtComponent save_selected_button   = newtButton(36, 6, "Save as selected...");
    newtComponent delete_selected_button = newtButton(36, 10, "  Delete selected  ");
    newtComponent close_button           = newtButton(36, 14, "       Close       ");
    newtComponent form = newtForm(NULL, NULL, 0);

    newtListboxSetWidth(selection_listbox, 31);

    for (size_t i = 0; i < data.size(); i++)
    {
        newtListboxAppendEntry(selection_listbox, timestamp2str(data.at(i).ent_time).c_str(), (void *)(i));
    }

    browse_networks_selection_callback_private dev_private(data,
                                                           start_date_label,
                                                           end_date_label,
                                                           architecture_label);

    newtComponentAddCallback(selection_listbox, browse_networks_selection_callback, &dev_private);

    newtFormAddComponents(form,
                          available_networks_label,
                          selection_listbox,
                          start_date_label,
                          end_date_label,
                          architecture_label,
                          save_selected_button,
                          delete_selected_button,
                          close_button,
                          NULL);

    struct newtExitStruct es;

    while (true)
    {
        newtFormRun(form, &es);

        if (es.reason == newtExitStruct::NEWT_EXIT_COMPONENT)
        {
            if (es.u.co == close_button)
            {
                break;
            }
            else if (es.u.co == delete_selected_button)
            {
                long int index = (long int)(newtListboxGetCurrent(selection_listbox));

                std::string question = std::string("Do you realy want do delete ");

                if (mode == mode_operation::PRODUCTION)
                {
                    question += std::string("PRODUCTION ");
                }
                else
                {
                    question += std::string("TESTNET ");
                }

                question += std::string("neural network generated at ")
                            + timestamp2str(data.at(index).ent_time)
                            + std::string("?\nNote, that after delete data cannot be recovered.");

                if (ask_question(question))
                {
                    try
                    {
                        db::delete_network(mode, data.at(index));
                        newtListboxDeleteEntry(selection_listbox, (void *)(index));

                        if (newtListboxItemCount(selection_listbox) == 0)
                        {
                            // If listbox is empty (no networks), close the window.
                            break;
                        }
                        else
                        {
                            index = (long int)(newtListboxGetCurrent(selection_listbox));

                            std::string start_date_text   = std::string("Start: ") + timestamp2str(data.at(index).start_time);
                            std::string end_date_text     = std::string("  End: ") + timestamp2str(data.at(index).ent_time);
                            std::string architecture_text = std::string("Architecture: ") + data.at(index).architecture;

                            newtLabelSetText(start_date_label, start_date_text.c_str());
                            newtLabelSetText(end_date_label, end_date_text.c_str());
                            newtLabelSetText(architecture_label, "                               ");
                            newtLabelSetText(architecture_label, architecture_text.c_str());
                        }
                    }
                    catch (const std::exception &e)
                    {
                        announcement("ERROR", e.what());
                    }
                }
            }
            else if (es.u.co == save_selected_button)
            {
                long int index = (long int)(newtListboxGetCurrent(selection_listbox));

                std::string file_name = save_as_file();

                if (file_name.length() > 0)
                {
                    try
                    {
                        file_put_contents(file_name, data.at(index).network);
                    }
                    catch (const std::exception &e)
                    {
                        announcement("ERROR", e.what());
                    }
                }
            }
        }
    }

    /* end dialog */
    newtFormDestroy(form);
    newtPopWindow();
}

} /* namespace dialogs */
} /* namespace bdm */
