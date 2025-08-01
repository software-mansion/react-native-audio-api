import type { Testimonial } from "./types";

const testimonials: Testimonial[] = [
  {
    author: 'Matt McGuiness',
    company: 'Perch',
    body: 'Perch has been using react-native-audio-api for streaming generated audio in our app and it’s been fantastic. The audio quality is crisp and features like variable playback speed work seamlessly without any hiccups. We’ve replaced all other audio libraries we were using with it.',
    image: {
      alt: 'Matt McGuiness',
      src: 'https://perch-app-prod.s3.us-east-1.amazonaws.com/media/matt-mcguiness.png',
    },
  },
  {
    author: 'Kim Chouard',
    company: 'Odisei Music',
    body: (
      <>
        <span>react-native-audio-api is a total game changer. It fills one of the biggest gaps in the React Native ecosystem by embracing what already works: the Web Audio API, making it universal and somehow even improving the Web in the process.</span>
        <br /> <br />
        <span>At Odisei Music, it powers all audio in Odisei Play, our saxophone learning app, where &lt;10ms latency isn’t a luxury, it’s a necessity. No other solution even came close.</span>
        <br /> <br />
        <span>This lib doesn’t just solve a problem, it unlocks a whole new category of apps for React Native devs.</span>
      </>
    ),
    link: 'https://odiseimusic.com/odisei-play/',
    image: {
      alt: 'Kim Chouard',
      src: '/react-native-audio-api/img/kim-chouard.jpg',
    },
  },
  {
    author: 'Daniel Zohar',
    company: 'Mindmax Labs',
    body: 'react-native-audio-api significantly accelerated our development, allowing us to ditch a bunch of libraries and custom code. The team has been super responsive and tackled all issues quickly.',
    image: {
      alt: 'Daniel Zohar',
      src: '/react-native-audio-api/img/daniel-zohar.jpeg',
    },
  }
];

export default testimonials;
