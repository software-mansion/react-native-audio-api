
export interface Testimonial {
  author: string;
  company: string;
  body: string | React.ReactNode;
  link?: string;
  image: {
    alt: string;
    src: string;
  };
}
