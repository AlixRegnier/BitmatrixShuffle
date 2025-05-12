#include <fast_median.h>

//https://rcoh.me/posts/linear-time-median-finding/

//Find median by sorting O(n.log(n))
//But assumed to be O(1) for n < 5
double nlogn_median(std::vector<double>& distances)
{
    //std::cout << "nlogn median" << std::endl;
    std::sort(distances.begin(), distances.end());
    if(distances.size() % 2 == 1)
        return distances[distances.size() / 2]; //Median
    else
        return 0.5 * (distances[distances.size() / 2 - 1] +
                    distances[distances.size() / 2]); //Average of two medians
}

double quickselect_median(std::vector<double>& distances)
{
    //std::cout << "quickselect_median " << distances.size() << std::endl;
    if(distances.size() % 2 == 1)
        return quickselect(distances, distances.size() / 2);
    else
        return 0.5 * (quickselect(distances, distances.size() / 2 - 1) +
                    quickselect(distances, distances.size() / 2));
}


//Used to find median in O(n)
double approximate_median(std::vector<double>& distances)
{
    //std::cout << "approximate_median " << distances.size() << std::endl;

    if(distances.size() < 5)
    {
        //std::cout << "approximate_median(nlogn_median)" << std::endl;
        return nlogn_median(distances);
    }

    std::vector<double> medians;
    medians.reserve(distances.size() / 5);

    for(auto i = distances.begin(); i < distances.end() - 4; i += 5)
    {
        std::sort(i, i+5);
        medians.push_back(*(i+2)); //Get middle among 5 --> 2-th element
    }

    //std::cout << "approximate_median(quickselect_median) " << medians.size() << std::endl;
    return quickselect_median(medians);
}

//Return the k-th largest element from vector
double quickselect(std::vector<double>& distances, unsigned k)
{
    if(distances.size() == 1)
        return distances[0];

    //std::cout << "quickselect1" << std::endl;
    //std::cout << distances.size() << std::endl;
    //Approximate median as pivot to prevent QuickSelect worst-case to occur (which is n^2)
    //std::cout << "quickselect(approximate median)" << std::endl;
    double pivot = approximate_median(distances);

    //Lows
    std::vector<double> lows;
    for(unsigned i = 0; i < distances.size(); ++i)
        if(distances[i] < pivot)
            lows.push_back(distances[i]);

    //if(k < lows.size())
        //std::cout << "quickselect(quickselect) : lows " << lows.size() << std::endl;
    if(k < lows.size())
        return quickselect(lows, k);

    //Pivots
    std::vector<double> pivots;
    for(unsigned i = 0; i < distances.size(); ++i)
        if(distances[i] == pivot)
            pivots.push_back(distances[i]);

    if(k < lows.size() + pivots.size())
        //std::cout << "quickselect: median guessed" << std::endl;
    if(k < lows.size() + pivots.size()) //We got lucky and guessed the median
        return pivots[0];

    //Highs
    std::vector<double> highs;
    highs.reserve(distances.size() - pivots.size() - lows.size());

    for(unsigned i = 0; i < distances.size(); ++i)
        if(distances[i] > pivot)
            highs.push_back(distances[i]);

    //std::cout << "quickselect(quickselect) : highs " << highs.size() << std::endl;
    return quickselect(highs, k - lows.size() - pivots.size());
}