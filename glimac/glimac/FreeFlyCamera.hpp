#pragma once

#include <vector>
#include "common.hpp"
#include "glm.hpp"

namespace glimac {

// Représente une camera qui cible un point précis

    class FreeFlyCamera {
    public:
        // Constructeur: alloue le tableau de données et construit les attributs des vertex
        FreeFlyCamera()
        {
            this->m_Position = glm::vec3(0.f);
            this->m_fPhi = glm::pi<float>();
            this->m_fTheta = 0.f;
        }

        float getAnglePhi()
        {
            return m_fPhi;
        }

        float getAngleTheta()
        {
            return m_fTheta;
        }

        void computeDirectionVectors()
        {
            float radPhi = m_fPhi;
            float radTheta = m_fTheta;
            m_FrontVector  = glm::vec3(glm::cos(radTheta) * glm::sin(radPhi), glm::sin(radTheta), glm::cos(radTheta) * glm::cos(radPhi));
            m_LeftVector   = glm::vec3(glm::sin(radPhi + (glm::pi<float>() / 2.f)), 0, glm::cos(radPhi + (glm::pi<float>() / 2.f)));
            m_UpVector     = glm::cross(m_FrontVector, m_LeftVector);
        }

        void moveLeft(float t){
            m_Position += t * m_LeftVector;
            computeDirectionVectors();
        }

        /// @brief Permet d'avancer / reculer la caméra de la distance delta (t positif avance la camera)
        /// @param t
        void moveFront(float t){
            m_Position += t * m_FrontVector;
            computeDirectionVectors();
        }

        /// @brief Permet de tourner horizontalement autour du centre de vision
        /// @param degrees
        void rotateLeft(float degrees)
        {
            m_fPhi = glm::radians(degrees);
            computeDirectionVectors();
        }

        /// @brief Permet de tourner verticalement autour du centre de vision
        /// @param degrees
        void rotateUp(float degrees)
        {
            m_fTheta = glm::radians(degrees);
            computeDirectionVectors();
        }

        glm::mat4 getViewMatrix() const
        {
            glm::mat4 ViewMatrix = glm::lookAt(m_Position, m_Position + m_FrontVector, m_UpVector);
            return ViewMatrix;
        }

        void setElevation(float e){
            m_Position.y = e;
            computeDirectionVectors();
        }

        void SetPosition(glm::vec3 pos)
        {
            m_Position = pos;
            computeDirectionVectors();
        }

        float getElevation(){
            return m_Position.y;
        }

    private:
        glm::vec3 m_Position; // position camera
        float m_fPhi; // angle autour de l'axe x (haut et bas)
        float m_fTheta; // angle autour de l'axe y (gauche et droite)

        glm::vec3 m_FrontVector;
        glm::vec3 m_LeftVector;
        glm::vec3 m_UpVector;
    };

} // namespace glimac