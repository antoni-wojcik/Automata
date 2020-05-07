//
//  camera.h
//  Automata
//
//  Created by Antoni Wójcik on 07/05/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef camera_h
#define camera_h

#include "camera.h"

#define ZOOM_MAX 0.05f
#define MOVE_SPEED 1.0f
#define ZOOM_SPEED 0.1f

class Camera {
private:
    float scr_width, scr_height;
    float width, height;
    float pos_x, pos_y;
    float zoom_current;
    
    void reposition() {
        if(pos_x < 0.0f) pos_x += scr_width;
        else if(pos_x > scr_width) pos_x -= scr_width;
        
        if(pos_y < 0.0f) pos_y += scr_width;
        else if(pos_y > scr_width) pos_y -= scr_width;
    }

public:
    Camera(unsigned int scr_width_n, unsigned int scr_height_n) {
        resize(scr_width_n, scr_height_n);
        
        pos_x = 0.0f;
        pos_y = 0.0f;
    }
    
    void zoom(float offset) {
        zoom_current -= offset * ZOOM_SPEED;
        
        if(zoom_current > 1.0f) zoom_current = 1.0f;
        else if(zoom_current < ZOOM_MAX) zoom_current = ZOOM_MAX;
        
        
        float width_old = width;
        float height_old = height;
        
        width = scr_width * zoom_current;
        height = scr_height * zoom_current;
        
        float width_diff = width_old - width;
        float height_diff = height_old - height;
        
        pos_x += width_diff * 0.5f;
        pos_y += height_diff * 0.5f;
        
        
        reposition();
    }
    
    void move(float offset_x, float offset_y) {
        pos_x += offset_x * zoom_current * MOVE_SPEED;
        pos_y += offset_y * zoom_current * MOVE_SPEED;
        
        reposition();
    }
    
    void resize(unsigned int scr_width_n, unsigned int scr_height_n) {
        scr_width = (float)(scr_width_n) * 0.5f;
        scr_height = (float)(scr_height_n) * 0.5f;
        
        width = scr_width;
        height = scr_height;
        zoom_current = 1.0f;
    }
    
    void transferData(Shader& shader, const std::string pos_x_id, const std::string pos_y_id, const std::string scr_width_id, const std::string scr_height_id) const {
        shader.use();
        
        shader.setFloat(pos_x_id, pos_x/scr_width);
        shader.setFloat(pos_y_id, pos_y/scr_width);
        shader.setFloat(scr_width_id, width/scr_width);
        shader.setFloat(scr_height_id, height/scr_height);
    }
};

#endif /* camera_h */
