#ifndef __NAOYA_MENU_H__
#define __NAOYA_MENU_H__

#include <MenuItemInterface.h>
#include <vector>

struct ChatMessage {
    String text;
    bool isUser;
};

class NaoyaMenu : public MenuItemInterface {
public:
    NaoyaMenu() : MenuItemInterface("Naoya AI") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return false; }
    void drawIconImg() override;
    bool getTheme() override;
    String themePath() { return ""; }

private:
    std::vector<ChatMessage> chatHistory;
    int scrollY = 0;
    bool inputSelected = false;
    void drawInterface();
    void drawChatBubbles();
    void sendRequest(String question);
    String getTimeStr();
};

#endif