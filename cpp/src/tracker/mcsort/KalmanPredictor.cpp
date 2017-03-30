#include "KalmanPredictor.h"

std::map<std::string, int> KalmanPredictor::classCount;

// Constructors

KalmanPredictor::KalmanPredictor(const Detection &initialState)
        : filter(nullptr),
          className(initialState.className),
          ID(++classCount[initialState.className]),
          timeSinceUpdate(0),
          hitStreak(0) {
    dlib::matrix<double, numStates, numStates> F; // System dynamics matrix
    dlib::matrix<double, numMeas, numStates> H; // Output matrix
    dlib::matrix<double, numStates, numStates> Q; // Process noise covariance
    dlib::matrix<double, numMeas, numMeas> R; // Measurement noise covariance
    dlib::matrix<double, numStates, numStates> P; // Estimate error covariance

    // define constant velocity model
    F = 1, 0, 0, 0, 1, 0, 0,
            0, 1, 0, 0, 0, 1, 0,
            0, 0, 1, 0, 0, 0, 1,
            0, 0, 0, 1, 0, 0, 0,
            0, 0, 0, 0, 1, 0, 0,
            0, 0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 0, 1;

    H = 1, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 0, 1, 0, 0, 0, 0,
            0, 0, 0, 1, 0, 0, 0;

    // Covariance values from SORT
    Q = 1, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 0, 1, 0, 0, 0, 0,
            0, 0, 0, 1, 0, 0, 0,
            0, 0, 0, 0, .01, 0, 0,
            0, 0, 0, 0, 0, .01, 0,
            0, 0, 0, 0, 0, 0, .0001;

    R = 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 10, 0,
            0, 0, 0, 10;

    P = 10, 0, 0, 0, 0, 0, 0,
            0, 10, 0, 0, 0, 0, 0,
            0, 0, 10, 0, 0, 0, 0,
            0, 0, 0, 10, 0, 0, 0,
            0, 0, 0, 0, 10000, 0, 0,
            0, 0, 0, 0, 0, 10000, 0,
            0, 0, 0, 0, 0, 0, 10000;

    filter = std::make_shared<dlib::kalman_filter<numStates, numMeas>>(dlib::kalman_filter<numStates, numMeas>());
    filter->set_transition_model(F);
    filter->set_observation_model(H);
    filter->set_process_noise(Q);
    filter->set_measurement_noise(R);
    filter->set_estimation_error_covariance(P);
    dlib::matrix<double, numMeas, 1> x0(boundingBoxToMeas(initialState.bb));
    filter->update(x0);
}

KalmanPredictor::KalmanPredictor(KalmanPredictor &&rhs)
        : filter(std::move(rhs.filter)),
          className(std::move(rhs.className)),
          ID(rhs.ID),
          timeSinceUpdate(rhs.timeSinceUpdate),
          hitStreak(rhs.hitStreak) {}

KalmanPredictor &KalmanPredictor::operator=(KalmanPredictor &&rhs) {
    filter = std::move(rhs.filter);
    className = std::move(rhs.className);
    ID = rhs.ID;
    timeSinceUpdate = rhs.timeSinceUpdate;
    hitStreak = rhs.hitStreak;
    return *this;
}

// Methods

void KalmanPredictor::update() {
    timeSinceUpdate++;
    hitStreak = 0;
    filter->update();
}

void KalmanPredictor::update(const Detection &det) {
    timeSinceUpdate = 0;
    hitStreak++;
    filter->update(boundingBoxToMeas(det.bb));
}

// Getters

Detection KalmanPredictor::getPredictedNextDetection() const {
    return Detection(className, stateToBoundingBox(filter->get_predicted_next_state()));
}

Tracking KalmanPredictor::getTracking() const {
    return Tracking(className, ID, stateToBoundingBox(filter->get_current_state()));
}

int KalmanPredictor::getTimeSinceUpdate() const {
    return timeSinceUpdate;
}

int KalmanPredictor::getHitStreak() const {
    return hitStreak;
}

const int KalmanPredictor::getID() const {
    return ID;
}

// Functions

dlib::matrix<double, KalmanPredictor::numMeas, 1> KalmanPredictor::boundingBoxToMeas(const BoundingBox &bb) {
    dlib::matrix<double, KalmanPredictor::numMeas, 1> z;
    z = bb.cx, bb.cy, bb.area(), bb.ratio();
    return z;
}

BoundingBox KalmanPredictor::stateToBoundingBox(const dlib::matrix<double, numStates, 1> &state) {
    double rectifiedArea = std::max(state(2), 0.);
    double width = std::sqrt(rectifiedArea * state(3));
    double height = rectifiedArea / width;
    return BoundingBox(state(0), state(1), width, height);
}

std::ostream &operator<<(std::ostream &os, const KalmanPredictor &kp) {
    os << kp.getPredictedNextDetection();
    return os;
}