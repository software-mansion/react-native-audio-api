import React, { useState, useEffect } from 'react';
import clsx from 'clsx';
import styles from './styles.module.css';
import TestimonialItem from '@site/src/components/Testimonials/TestimonialItem';

interface Testimonial {
  author: string;
  company: string;
  body: string;
  link?: string;
  image: {
    alt: string;
    src: string;
  };
}

const items: Testimonial[] = [
  {
    author: 'Matt McGuiness',
    company: 'Perch',
    body: 'Perch has been using react-native-audio-api for streaming generated audio in our app and it’s been fantastic. The audio quality is crisp and features like variable playback speed work seamlessly without any hiccups. We’ve replaced all other audio libraries we were using with it.',
    // link: 'https://twitter.com/mrousavy/status/1754909520571019756',
    image: {
      alt: 'Matt McGuiness',
      src: 'https://perch-app-prod.s3.us-east-1.amazonaws.com/media/matt-mcguiness.png',
    },
  },
  {
    author: 'Jakiś Gerwim',
    company: 'Companicus Anonymus',
    body: `We have implemented react-native-audio-api in our app because we need a performant solution with millisecond precision timing when playing multiple tracks. The expert team of Software Mansion are a delight to work with.`,
    image: {
      alt: 'Jakiś Gerwim',
      src: 'https://s.ciekawostkihistoryczne.pl/uploads/2017/12/Gall-Anonim-340x340.jpg',
    },

  }
];

const TestimonialList = () => {
  const [activeIndex, setActiveIndex] = useState(0);

  useEffect(() => {
    const updateHeight = () => {
      const testimonialContainer = document.querySelector<HTMLElement>(
        `.testimonialContainer-${activeIndex}`
      );
      const testimonialSlides =
        document.querySelector<HTMLElement>('.testimonialSlides');
      if (
        testimonialContainer.childElementCount === 1 &&
        testimonialSlides.offsetHeight > testimonialContainer.offsetHeight
      ) {
        return;
      }
      testimonialSlides.style.height = `${testimonialContainer.offsetHeight}px`;
    };

    updateHeight();

    window.addEventListener('resize', updateHeight);
    return () => {
      window.removeEventListener('resize', updateHeight);
    };
  }, [activeIndex]);

  const handleDotClick = (index) => {
    setActiveIndex(index);
  };

  const renderedItems = [];
  for (let i = 0; i < items.length; i += 1) {
    renderedItems.push(
      <div
        className={clsx(
          `testimonialContainer-${i}`,
          styles.testimonialPair
        )}
        key={i}>
        <TestimonialItem
          company={items[i].company}
          image={items[i].image}
          link={items[i].link}
          author={items[i].author}>
          {items[i].body}
        </TestimonialItem>
      </div>
    );
  }

  return (
    <div className={styles.testimonialSlides}>
      <div className="testimonialSlides">
        {renderedItems.map((item, idx) => (
          <div
            key={idx}
            className={clsx(
              styles.testimonialSlide,
              activeIndex === idx ? styles.activeTestimonialSlide : ''
            )}>
            {item}
          </div>
        ))}
      </div>
      <div className={styles.dotsContainer}>
        {renderedItems.map((_, idx) => (
          <span
            key={idx}
            className={clsx(
              styles.dot,
              idx === activeIndex && styles.activeDot
            )}
            onClick={() => handleDotClick(idx)}
          />
        ))}
      </div>
    </div>
  );
};

export default TestimonialList;
