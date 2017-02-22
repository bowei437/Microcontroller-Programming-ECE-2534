int getButton(int buttonIndex) {

    enum States {INIT_STATE, READ_BUTTON, INCR_COUNT, CLEAR_COUNT, DONE_STATE};
    const int MAX_COUNT = 1000;

    int button, prevButton, count;
    enum States state = INIT_STATE;

    while (state != DONE_STATE) {
        switch (state) {
            case INIT_STATE:
                button = readButton(buttonIndex);
                count = 0;
                state = READ_BUTTON;
                break;
            case READ_BUTTON:
                prevButton = button;
                button = readButton(buttonIndex);
                if (button == prevButton)
                    state = INCR_COUNT;
                else
                    state = CLEAR_COUNT;
                break;
            case INCR_COUNT:
                count++;
                if (count < MAX_COUNT)
                    state = READ_BUTTON;
                else
                    state = DONE_STATE;
                break;
            case CLEAR_COUNT:
                count = 0;
                state = READ_BUTTON;
                break;
            case DONE_STATE:
                break;
            default:
                break;
        }
    }
    
    return button;
}
